#include "globals.h"
#include "parser.h"
#include <ctype.h>
#include <errno.h>

#include "iburgcodes-coh.h"

static Symbol* symbol_table = NULL;
static const char* SYM_REGISTER = "register";
static const char* SYM_REGCLASS = "register class";

static Rule* rules[500];
static int rulescount = 0;
static int registercount = 0;
static reg_t operandregs = 0;

static int maxdepth = 0;
static Node* pattern;

static const char* infilename;
FILE* outfp;
FILE* outhfp;

const char* machine_word = NULL;

void* lookup_symbol(const char* name, const char* kind)
{
	Symbol* s = symbol_table;
	while (s)
	{
		if (strcmp(s->name, name) == 0)
		{
			if (kind && (s->kind != kind))
				fatal("symbol '%s' is not a %s", name, kind);
			return s;
		}
		s = s->next;
	}
	return NULL;
}

void* define_symbol(const char* name, const char* kind, size_t size)
{
	if (lookup_symbol(name, kind))
		fatal("symbol '%s' already exists", name);
	Symbol* s = calloc(size, 1);
	s->name = name;
	s->kind = kind;
	s->next = symbol_table;
	symbol_table = s;
	return s;
}

Register* define_register(const char* name)
{
	Register* reg = define_symbol(name, SYM_REGISTER, sizeof(Register));
	reg->compatible = reg->uses = reg->id = 1<<registercount;
	registercount++;
	return reg;
}

Register* lookup_register(const char* name)
{
	Register* reg = lookup_symbol(name, SYM_REGISTER);
	if (!reg)
		fatal("unknown register '%s'", name);
	return reg;
}

Register* lookup_register_by_id(reg_t id)
{
	Register* reg = (Register*) symbol_table;
	while (reg)
	{
		if ((reg->sym.kind == SYM_REGISTER) && (reg->id == id))
			return reg;
		reg = (Register*) reg->sym.next;
	}
	fatal("unknown register 0x%x", id);
}

reg_t lookup_register_or_class(const char* name)
{
	Symbol* s = lookup_symbol(name, NULL);
	if (s)
	{
		if (s->kind == SYM_REGISTER)
			return ((Register*)s)->id;
		if (s->kind == SYM_REGCLASS)
			return ((RegisterClass*)s)->reg;
	}
	fatal("'%s' is not a register or register class", name);
}

static reg_t find_conflicting_registers(reg_t id)
{
	reg_t conflicting = 0;
	Register* reg = (Register*) symbol_table;
	while (reg)
	{
		if ((reg->sym.kind == SYM_REGISTER) && (reg->id & id))
			conflicting |= reg->uses;
		reg = (Register*) reg->sym.next;
	}
	return conflicting;
}

void define_operand(const char* name)
{
	Register* r = define_register(name);
	r->isstacked = true;
	r->isoperand = true;
	operandregs |= r->id;
}

void define_regclass(const char* name, reg_t reg)
{
	RegisterClass* rc = define_symbol(name, SYM_REGCLASS, sizeof(RegisterClass));
	rc->reg = reg;
}

int lookup_midcode(const char* name)
{
	/* TODO: these are not terminals, change the name */
	for (int i=0; i<sizeof(terminals)/sizeof(*terminals); i++)
	{
		const char* t = terminals[i];
		if (strcmp(name, t) == 0)
			return i; 
	}
	yyerror("unknown midcode '%s'", name);
	return 0;
}

Node* register_matcher(reg_t reg, Label* label)
{
	Node* n = calloc(sizeof(Node), 1);
	n->isregister = true;
	n->reg = reg;
	n->label = label;
	return n;
}

Node* tree_matcher(int midcode, Node* left, Node* right, Predicate* predicate, Label* label)
{
	Node* n = calloc(sizeof(Node), 1);
	n->isregister = false;
	n->midcode = midcode;
	n->left = left;
	n->right = right;
	n->predicate = predicate;
	n->label = label;
	return n;
}

Rule* genrule(int lineno, Node* pattern, reg_t result)
{
	if (pattern->isregister)
		yyerror("trivial rule");

	Rule* r = calloc(sizeof(Rule), 1);
	r->lineno = lineno;
	r->pattern = pattern;
	r->result_reg = result;

	if (rulescount == (sizeof(rules)/sizeof(*rules)))
		yyerror("too many rules");
	rules[rulescount++] = r;
	return r;
}

static int collect_template_data(Node* template, Node* pattern, Label** last_label)
{
	if (!template)
		return 0;
	if (template->label)
	{
		template->label->next = *last_label;
		*last_label = template->label;
	}

	int cost = !(template->isregister) + !!(template->predicate);
	if (template->left)
	{
		if (!pattern->left)
		{
			Node* p = calloc(sizeof(Node), 1);
			pattern->left = p;
		}
		cost += collect_template_data(template->left, pattern->left, last_label);
	}
	if (template->right)
	{
		if (!pattern->right)
		{
			Node* p = calloc(sizeof(Node), 1);
			pattern->right = p;
		}
		cost += collect_template_data(template->right, pattern->right, last_label);
	}
	return cost;
}

static int sort_rule_cb(const void* left, const void* right)
{
	const Rule* const* r1 = left;
	const Rule* const* r2 = right;
	if ((*r1)->cost > (*r2)->cost)
		return -1;
	if ((*r1)->cost < (*r2)->cost)
		return 1;
	return 0;
}

static void calculate_pattern_size(Node* node)
{
	maxdepth++;
	if (node->left)
		calculate_pattern_size(node->left);
	if (node->right)
		calculate_pattern_size(node->right);
}

static Node* lookup_label(Node* node, const char* name)
{
	Node* f = NULL;
	if (!node)
		return f;
	else if (node->label && (strcmp(node->label->name, name) == 0))
		f = node;
	else
	{
		f = lookup_label(node->left, name);
		if (!f)
			f = lookup_label(node->right, name);
	}
	return f;
}

static void resolve_label_names(Rule* rule)
{
	Label* label = rule->first_label;
	while (label)
	{
		/* Check the name of this label doesn't conflict with any other
		 * on the rule. */

		Label* other = label->next;
		while (other)
		{
			if (strcmp(label->name, other->name) == 0)
				fatal("duplicate label '%s'", label->name);
			other = other->next;
		}

		label = label->next;
	}
}

static void copy_nodes(int* offset, Rule* rule, Node* template, Node* pattern)
{
	int thisoffset = *offset;
	if (template)
	{
		template->index = thisoffset;
		rule->nodes[thisoffset] = template;
		if (template->predicate)
			rule->has_predicates = true;
	}

	if (pattern->left)
	{
		(*offset)++;
		copy_nodes(offset, rule, template ? template->left : NULL, pattern->left);
	}
	if (pattern->right)
	{
		(*offset)++;
		copy_nodes(offset, rule, template ? template->right : NULL, pattern->right);
	}
}

static void sort_rules(void)
{
	pattern = calloc(sizeof(Node), 1);

	for (int i=0; i<rulescount; i++)
	{
		Rule* r = rules[i];
		r->cost += collect_template_data(r->pattern, pattern, &r->first_label);
	}

	qsort(rules, rulescount, sizeof(Rule*), sort_rule_cb);

	calculate_pattern_size(pattern);
	if (maxdepth > 32) 
		fatal("rules are too complex");

	for (int i=0; i<rulescount; i++)
	{
		Rule* r = rules[i];
		r->nodes = calloc(maxdepth, sizeof(Node*));

		int offset = 0;
		copy_nodes(&offset, r, r->pattern, pattern);

		resolve_label_names(r);

		r->compatible_regs = 0;
		Register* reg = (Register*) symbol_table;
		while (reg)
		{
			if ((reg->sym.kind == SYM_REGISTER) && (r->result_reg & reg->id))
				r->compatible_regs |= reg->compatible;
			reg = (Register*) reg->sym.next;
		}
	}
}

static void print_upper(FILE* fp, const char* s)
{
	while (*s)
		fputc(toupper(*s++), fp);
}

static void dump_registers(void)
{
	Register* reg = (Register*) symbol_table;
	while(reg)
	{
		if (reg->sym.kind == SYM_REGISTER)
		{
			fprintf(outhfp, "const REG_");
			print_upper(outhfp, reg->sym.name);
			fprintf(outhfp, " := 0x%x;\n", reg->id);
		}
		else if (reg->sym.kind == SYM_REGCLASS)
		{
			fprintf(outhfp, "const REGCLASS_");
			print_upper(outhfp, reg->sym.name);
			fprintf(outhfp, " := 0x%x;\n", reg->id);
		}
		reg = (Register*) reg->sym.next;
	}

	fprintf(outhfp, "var registers: Register[] := {\n");

	reg = (Register*) symbol_table;
	while(reg)
	{
		if (reg->sym.kind == SYM_REGISTER)
		{
			fprintf(outhfp, "\t{ \"%s\", 0x%x, 0x%x, 0x%x, %d },\n",
				reg->sym.name, reg->id, reg->uses, reg->compatible, reg->isstacked);
		}
		reg = (Register*) reg->sym.next;
	}
	fprintf(outhfp, "};\n");
}

static const char* operator_name(int operator)
{
	switch (operator)
	{
		case EQOP: return "==";
		case NEOP: return "!=";
		case LTOP: return "<";
		case LEOP: return "<=";
		case GTOP: return ">";
		case GEOP: return ">=";
	}
	assert(false);
}

static void print_lower(const char* s)
{
	while (*s)
		fputc(tolower(*s++), outfp);
}

static void print_predicate(int index, bool* first, Node* template, Predicate* predicate)
{
	while (predicate)
	{
		if (!*first)
			fprintf(outfp, " and");
		*first = false;

		switch (predicate->operator)
		{
			case IS:
				fprintf(outfp, " (is_%s(%sslots[%d].", predicate->u.callback, predicate->deref ? "&" : "", index);
				print_lower(midcodetypes[template->midcode]);
				fprintf(outfp, ".%s) != 0)", predicate->field);
				break;

			default:
				fprintf(outfp, " (%sslots[%d].", predicate->deref ? "&" : "", index);
				print_lower(midcodetypes[template->midcode]);
				fprintf(outfp, ".%s %s %d)",
					predicate->field,
					operator_name(predicate->operator),
					predicate->u.value);
				break;
		}

		predicate = predicate->next;
	}
}

static void create_match_predicates(void)
{
	fprintf(outfp, "sub MatchPredicate(rule: uint8, n: [[Node]]): (matches: uint8) is\n");
	fprintf(outfp, "var slots: [Node][%d];\n", maxdepth);
	fprintf(outfp, "MemCopy(n as [uint8], @bytesof slots, &slots[0] as [uint8]);\n");
	fprintf(outfp, "matches := 0;\n");
	fprintf(outfp, "case rule is\n");

	for (int i=0; i<rulescount; i++)
	{
		Rule* r = rules[i];
		if (r->has_predicates)
		{
			fprintf(outfp, "when %d:\n", i);
			fprintf(outfp, "if ");

			bool first = true;
			for (int j=0; j<maxdepth; j++)
			{
				Node* n = r->nodes[j];
				if (n)
					print_predicate(j, &first, n, n->predicate);
			}
			fprintf(outfp, " then matches := 1; end if;\n");
		}
	}

	fprintf(outfp, "end case;\n");
	fprintf(outfp, "end sub;\n");
}

static void create_rules(void)
{
	fprintf(outfp, "var codegen_midcodes: uint8[] := {\n");
	for (int i=0; i<rulescount; i++)
	{
		Rule* r = rules[i];
		for (int j=0; j<maxdepth; j++)
		{
			Node* n = r->nodes[j];
			if (n && n->midcode)
				fprintf(outfp, "% 3d, ", n->midcode);
		}
		fprintf(outfp, "# %d\n", i);
	}
	fprintf(outfp, "};\n");
	fprintf(outfp, "var codegen_registers: RegId[] := {\n");
	for (int i=0; i<rulescount; i++)
	{
		Rule* r = rules[i];
		for (int j=0; j<maxdepth; j++)
		{
			Node* n = r->nodes[j];
			if (n && n->isregister)
				fprintf(outfp, "0x%x, ", n->reg);
		}
		fprintf(outfp, "# %d\n", i);
	}
	fprintf(outfp, "};\n");

	fprintf(outfp, "var codegen_rules: Rule[] := {\n");

	for (int i=0; i<rulescount; i++)
	{
		Rule* r = rules[i];
		fprintf(outfp, "\t{ ");

		uint8_t flags = 0;
		if (r->has_predicates)
			flags |= 0x01;
		if (r->replacement)
			flags |= 0x02;
		fprintf(outfp, "0x%02x, ", flags);

		uint32_t copymask = 1;
		uint32_t regmask = 0;
		for (int j=1; j<maxdepth; j++)
		{
			if (r->nodes[j])
			{
				copymask |= 1<<j;
				if (r->nodes[j]->isregister)
					regmask |= 1<<j;
			}
		}

		uint32_t significantmask = 0;
		for (int j=0; j<maxdepth; j++)
		{
			Node* n = r->nodes[j];
			if (n && n->midcode)
				significantmask |= 1<<j;
		}

		fprintf(outfp, "0x%x, ", r->compatible_regs);
		fprintf(outfp, "0x%x, ", r->result_reg);
		fprintf(outfp, "0x%x, ", find_conflicting_registers(r->uses_regs));
		fprintf(outfp, "%d, ", significantmask);
		fprintf(outfp, "%d, %d ", copymask, regmask);
		fprintf(outfp, "}, ");
		fprintf(outfp, "# %d\n", i);
	}

	fprintf(outfp, "};\n");
}

static void print_complex_action(Rule* r, Element* e)
{
	if (e->next)
		print_complex_action(r, e->next);

	switch (e->kind)
	{
		case ELEMENT_STRING:
			fprintf(outfp, "%s", e->text);
			break;

		case ELEMENT_REGLABEL:
			if (e->text[0] == '$')
				fprintf(outfp, "selfreg");
			else
			{
				Node* node = lookup_label(r->pattern, e->text);
				if (!node)
					fatal("nothing labelled '%s' at line %d", e->text, r->lineno);

				if (node->isregister)
					fprintf(outfp, "slots[%d].reg", node->index);
				else
				{
					fprintf(outfp, "slots[%d].node.", node->index);
					print_lower(midcodetypes[node->midcode]);
				}
			}
			break;

		case ELEMENT_NODELABEL:
			if (e->text[0] == '$')
				fprintf(outfp, "slots[0].node");
			else
			{
				Node* node = lookup_label(r->pattern, e->text);
				if (!node)
					fatal("nothing labelled '%s' at line %d", e->text, r->lineno);

				fprintf(outfp, "slots[%d].node", node->index);
			}
			break;
	}
}

static void print_line(int lineno)
{
	fprintf(outfp, "#line %d \"%s\"\n", lineno+1, infilename);
}

static void create_emitters(void)
{
	fprintf(outfp, "sub EmitOneInstruction(ruleid: uint8, self: [Instruction]) is\n");
	fprintf(outfp, "record NodeSlot is\n");
	fprintf(outfp, "\tnode: [Node];\n");
	fprintf(outfp, "\treg: RegId;\n");
	fprintf(outfp, "end record;\n");
	fprintf(outfp, "var slots: NodeSlot[%d];\n", maxdepth);
	fprintf(outfp, "var psrc := &self.n[0];\n");
	fprintf(outfp, "var pdest := &slots[0];\n");
	fprintf(outfp, "var i: uint8 := %d;\n", maxdepth);
	fprintf(outfp, "while i != 0 loop\n");
	fprintf(outfp, "\tpdest.node := [psrc];\n");
	fprintf(outfp, "\tif pdest.node != (0 as [Node]) then\n");
	fprintf(outfp, "\t\tpdest.reg := pdest.node.produced_reg;\n");
	fprintf(outfp, "\tend if;\n");
	fprintf(outfp, "\tpsrc := @next psrc;\n");
	fprintf(outfp, "\tpdest := @next pdest;\n");
	fprintf(outfp, "\ti := i - 1;\n");
	fprintf(outfp, "end loop;\n");
	fprintf(outfp, "var selfreg := self.produced_reg;\n");
	fprintf(outfp, "interface Emitter();\n");

	for (int i=0; i<rulescount; i++)
	{
		Rule* r = rules[i];
		if (r->action)
		{
			fprintf(outfp, "sub emit_%d implements Emitter is\n", i);

			print_line(r->lineno);

			Action* a = r->action;
			if (a)
			{
				if (a->iscomplex)
					print_complex_action(r, a->first_element);
				else
					fatal("simple actions not supported yet");
			}

			fprintf(outfp, "\nend sub;\n");
		}
	}

	fprintf(outfp, "sub nop_emitter implements Emitter is end sub;\n");
	fprintf(outfp, "var emitters: Emitter[] := {\n");
	for (int i=0; i<rulescount; i++)
	{
		Rule* r = rules[i];
		if (r->action)
			fprintf(outfp, "\temit_%d,\n", i);
		else
			fprintf(outfp, "\tnop_emitter,\n");
	}
	fprintf(outfp, "};\n");

	fprintf(outfp, "(emitters[ruleid as @indexof emitters])();\n");
	fprintf(outfp, "end sub;\n");
}

static void emit_replacement(Rule* rule, Node* pattern, Node* replacement)
{
	if (replacement->isregister)
	{
		Node* node = lookup_label(pattern, replacement->label->name);
		if (!node)
			fatal("nothing labelled '%s' at line %d",
				replacement->label->name, rule->lineno);

		fprintf(outfp, "n[%d]", node->index);
	}
	else
	{
		fprintf(outfp, "mid_");
		print_lower(terminals[replacement->midcode]);
		fprintf(outfp, "(");

		if (replacement->left)
		{
			emit_replacement(rule, pattern, replacement->left);
			if (replacement->right)
			{
				fprintf(outfp, ", ");
				emit_replacement(rule, pattern, replacement->right);
			}
		}
		fprintf(outfp, ")");
	}
}

static void walk_matcher_tree(int* offset, Node* pattern)
{
	int thisoffset = *offset;
	fprintf(outfp, "\t[matchbuf+%d] := [n + %d*@bytesof intptr].op;\n", thisoffset, thisoffset);

	if (pattern->left)
	{
		(*offset)++;
		fprintf(outfp, "\t[n + %d*@bytesof intptr] := [n + %d*@bytesof intptr].left;\n", *offset, thisoffset);
		fprintf(outfp, "\tif [n + %d*@bytesof intptr] != (0 as [Node]) then\n", *offset);
		walk_matcher_tree(offset, pattern->left);
		fprintf(outfp, "\tend if;\n");
	}
	if (pattern->right)
	{
		(*offset)++;
		fprintf(outfp, "\t[n + %d*@bytesof intptr] := [n + %d*@bytesof intptr].right;\n", *offset, thisoffset);
		fprintf(outfp, "\tif [n + %d*@bytesof intptr] != (0 as [Node]) then\n", *offset);
		walk_matcher_tree(offset, pattern->right);
		fprintf(outfp, "\tend if;\n");
	}
}

static void create_matcher(void)
{
	fprintf(outfp, "sub PopulateMatchBuffer(insn: [Instruction], n: [[Node]], matchbuf: [uint8]) is\n");

	int offset = 0;
	walk_matcher_tree(&offset, pattern);

	fprintf(outfp, "end sub;\n");

}

int main(int argc, const char* argv[])
{
	if (argc != 4)
		fatal("syntax: newgen <inputfile> <output c file> <output h file>");

	infilename = argv[1];
	FILE* infp = fopen(infilename, "r");
	if (!infp)
		fatal("cannot open input file '%s': %s", infilename, strerror(errno));

	outfp = fopen(argv[2], "w");
	if (!outfp)
		fatal("cannot open output C file '%s': %s", argv[2], strerror(errno));

	outhfp = fopen(argv[3], "w");
	if (!outhfp)
		fatal("cannot open output H file '%s': %s", argv[3], strerror(errno));

	include_file(open_file(infp));
	define_regclass("all", -1);
	parse();

	sort_rules();

	if (registercount > 32)
		fatal("too many registers; %d, maximum 32", registercount);

	if (machine_word)
		fprintf(outhfp, "typedef Word is %s;\n", machine_word);

	if (operandregs == 0)
		fprintf(outhfp, "record Operand is end record;\n");

	fprintf(outhfp, "const INSTRUCTION_TEMPLATE_DEPTH := %d;\n", maxdepth);
	fprintf(outhfp, "const INSTRUCTION_TEMPLATE_COUNT := %d;\n", rulescount);
	fprintf(outhfp, "const REGISTER_COUNT := %d;\n", registercount);
	fprintf(outhfp, "const ALL_REGS := 0x%x;\n", ((uint64_t)1<<registercount) - 1);
	fprintf(outhfp, "const OPERAND_REGS := 0x%x;\n", operandregs);
	if (registercount == 32)
		fprintf(outhfp, "typedef RegId is uint32;\n");
	else
		fprintf(outhfp, "typedef RegId is int(0, ALL_REGS);\n");
	fprintf(outhfp, "typedef NodeBitmap is int(0, 0x%x);\n", (1<<maxdepth) - 1);

	fprintf(outhfp, "record Register is\n");
	fprintf(outhfp, "	name: string;\n");
	fprintf(outhfp, "	id: RegId;\n");
	fprintf(outhfp, "	uses: RegId;\n");
	fprintf(outhfp, "	compatible: RegId;\n");
	fprintf(outhfp, "	is_stacked: uint8;\n");
	fprintf(outhfp, "end record;\n");

	dump_registers();
	create_match_predicates();
	create_emitters();
	create_rules();
	create_matcher();

	return errcnt>0;
}

