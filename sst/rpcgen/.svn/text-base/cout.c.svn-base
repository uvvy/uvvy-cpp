/*
 * Sun RPC is a product of Sun Microsystems, Inc. and is provided for
 * unrestricted use provided that this legend is included on all tape
 * media and as a part of the software program in whole or part.  Users
 * may copy or modify Sun RPC without charge, but are not authorized
 * to license or distribute it to anyone else except as part of a product or
 * program developed by the user.
 *
 * SUN RPC IS PROVIDED AS IS WITH NO WARRANTIES OF ANY KIND INCLUDING THE
 * WARRANTIES OF DESIGN, MERCHANTIBILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE, OR ARISING FROM A COURSE OF DEALING, USAGE OR TRADE PRACTICE.
 *
 * Sun RPC is provided with no support and without any obligation on the
 * part of Sun Microsystems, Inc. to assist in its use, correction,
 * modification or enhancement.
 *
 * SUN MICROSYSTEMS, INC. SHALL HAVE NO LIABILITY WITH RESPECT TO THE
 * INFRINGEMENT OF COPYRIGHTS, TRADE SECRETS OR ANY PATENTS BY SUN RPC
 * OR ANY PART THEREOF.
 *
 * In no event will Sun Microsystems, Inc. be liable for any lost revenue
 * or profits or other special, indirect and consequential damages, even if
 * Sun has been advised of the possibility of such damages.
 *
 * Sun Microsystems, Inc.
 * 2550 Garcia Avenue
 * Mountain View, California  94043
 */

/*
 * cout.c, XDR routine outputter for the RPC protocol compiler
 * Copyright (C) 1987, Sun Microsystems, Inc.
 */
#include <ctype.h>
#include <stdio.h>
#include <string.h>
#include "parse.h"
#include "util.h"

static void print_trailer( void );
static void print_stat( int , declaration * , int );
static void emit_enum( definition * );
static void emit_program( definition * );
static void emit_union( definition * );
static void emit_struct( definition * );
static void emit_typedef( definition * );
static void emit_inline( int, declaration *, int );
static void emit_single_in_line( int, declaration *, int, relation );

/*
 * Emit the C-routine for the given definition
 */
void
emit(def)
	definition *def;
{
	if (def->def_kind == DEF_CONST) {
		return;
	}
	if (def->def_kind == DEF_PROGRAM) {
		//emit_program(def);
		return;
	}
	if (def->def_kind == DEF_TYPEDEF) {
		/*
		 * now we need to handle declarations like
		 * struct typedef foo foo;
		 * since we dont want this to be expanded into 2 calls to xdr_foo
		 */

		if (strcmp(def->def.ty.old_type, def->def_name) == 0)
			return;
	};
	switch (def->def_kind) {
	case DEF_UNION:
		emit_union(def);
		break;
	case DEF_ENUM:
		emit_enum(def);
		break;
	case DEF_STRUCT:
		emit_struct(def);
		break;
	case DEF_TYPEDEF:
		emit_typedef(def);
		break;
		/* DEF_CONST and DEF_PROGRAM have already been handled */
	default:
		break;
	}
}

static int
findtype(def, type)
	definition *def;
	char *type;
{

	if (def->def_kind == DEF_PROGRAM || def->def_kind == DEF_CONST) {
		return (0);
	} else {
		return (streq(def->def_name, type));
	}
}

static int
undefined(type)
	char *type;
{
	definition *def;

	def = (definition *) FINDVAL(defined, type, findtype);
	return (def == NULL);
}


static void
print_header(char *procname, int encode)
{
	f_print(fout, "\n");
	f_print(fout, "XdrStream &operator%s(XdrStream &xs, %s%s &v)\n",
		encode ? "<<" : ">>", encode ? "const " : "", procname);
	f_print(fout, "{\n");
}

static void
print_trailer()
{
	f_print(fout, "\treturn xs;\n");
	f_print(fout, "}\n");
}


static void
print_ifopen(indent, name)
	int indent;
	char *name;
{
	tabify(fout, indent);
	f_print(fout, "if (!xdr_%s(xdrs", name);
}

static void
print_ifarg(arg)
	char *arg;
{
	f_print(fout, ", %s", arg);
}

static void
print_ifsizeof(indent, prefix, type)
	int indent;
	char *prefix;
	char *type;
{
	if (indent) {
		f_print(fout, ",\n");
		tabify(fout, indent);
	} else  {
		f_print(fout, ", ");
	}
	if (streq(type, "bool")) {
		f_print(fout, "sizeof (bool_t), (xdrproc) xdr_bool");
	} else {
		f_print(fout, "sizeof (");
		if (undefined(type) && prefix) {
			f_print(fout, "%s ", prefix);
		}
		f_print(fout, "%s), (xdrproc) xdr_%s", type, type);
	}
}

static void
print_ifclose(indent)
	int indent;
{
	f_print(fout, "))\n");
	tabify(fout, indent);
	f_print(fout, "\treturn (XDR_FALSE);\n");
}

static void
print_ifstat(indent, prefix, type, rel, amax, objname, name)
	int indent;
	char *prefix;
	char *type;
	relation rel;
	char *amax;
	char *objname;
	char *name;
{
	char *alt = NULL;

	switch (rel) {
	case REL_POINTER:
		print_ifopen(indent, "pointer");
		print_ifarg("(char **)");
		f_print(fout, "%s", objname);
		print_ifsizeof(0, prefix, type);
		break;
	case REL_OPTION:
		print_ifopen(indent, "option");
		print_ifarg("(void **)");
		if (*objname == '&') {
			f_print(fout, "%s.val", objname);
			print_ifsizeof(0, prefix, type);
			f_print(fout, ", %s.xdr, %s.len", objname, objname);
		} else {
			f_print(fout, "&%s->val", objname);
			print_ifsizeof(0, prefix, type);
			f_print(fout, ", &%s->xdr, &%s->len", objname, objname);
		}
		break;
	case REL_VECTOR:
		if (streq(type, "string")) {
			alt = "string";
		} else if (streq(type, "opaque")) {
			alt = "opaque";
		}
		if (alt) {
			print_ifopen(indent, alt);
			print_ifarg(objname);
		} else {
			print_ifopen(indent, "vector");
			print_ifarg("(char *)");
			f_print(fout, "%s", objname);
		}
		print_ifarg(amax);
		if (!alt) {
			print_ifsizeof(indent + 1, prefix, type);
		}
		break;
	case REL_ARRAY:
		if (streq(type, "string")) {
			alt = "string";
		} else if (streq(type, "opaque")) {
			alt = "bytes";
		}
		if (streq(type, "string")) {
			print_ifopen(indent, alt);
			print_ifarg(objname);
		} else {
			if (alt) {
				print_ifopen(indent, alt);
			} else {
				print_ifopen(indent, "array");
			}
			print_ifarg("(void **)");
			if (*objname == '&') {
				f_print(fout, "%s.%s_val, (uint32_t *) %s.%s_len",
					objname, name, objname, name);
			} else {
				f_print(fout,
					"&%s->%s_val, (uint32_t *) &%s->%s_len",
					objname, name, objname, name);
			}
		}
		print_ifarg(amax);
		if (!alt) {
			print_ifsizeof(indent + 1, prefix, type);
		}
		break;
	case REL_ALIAS:
		print_ifopen(indent, type);
		print_ifarg(objname);
		break;
	}
	print_ifclose(indent);
}

static void
print_encstat(int indent, relation rel, char *amax, char *objname)
{
	tabify(fout, indent);
	switch (rel) {
	case REL_ALIAS:
	case REL_POINTER:
	case REL_OPTION:
		f_print(fout, "xs << %s;\n", objname);
		break;
	case REL_VECTOR:
		f_print(fout, "xdrEncodeVector(xs, %s, %s);\n", objname, amax);
		break;
	case REL_ARRAY:
		f_print(fout, "xdrEncodeArray(xs, %s, %s);\n", objname, amax);
		break;
	}
}

static void
print_decstat(int indent, relation rel, char *amax, char *objname)
{
	tabify(fout, indent);
	switch (rel) {
	case REL_ALIAS:
	case REL_POINTER:
	case REL_OPTION:
		f_print(fout, "xs >> %s;\n", objname);
		break;
	case REL_VECTOR:
		f_print(fout, "xdrDecodeVector(xs, %s, %s);\n", objname, amax);
		break;
	case REL_ARRAY:
		f_print(fout, "xdrDecodeArray(xs, %s, %s);\n", objname, amax);
		break;
	}
}

static void
print_stat(int indent, declaration *dec, int encode)
{
	char objname[256];

	s_print(objname, "v.%s", dec->name);
	if (encode)
		print_encstat(indent, dec->rel, dec->array_max, objname);
	else
		print_decstat(indent, dec->rel, dec->array_max, objname);
}

/* ARGSUSED */
static void
emit_enum(definition *def)
{
	print_header(def->def_name, 1);
	f_print(fout, "\txs << (qint32)v;\n");
	print_trailer();

	print_header(def->def_name, 0);
	f_print(fout, "\tqint32 i;\n");
	f_print(fout, "\txs >> i;\n");
	f_print(fout, "\tv = (%s)i;\n", def->def_name);
	print_trailer();
}

static void
emit_program(def)
	definition *def;
{
	decl_list *dl;
	version_list *vlist;
	proc_list *plist;

	for (vlist = def->def.pr.versions; vlist != NULL; vlist = vlist->next)
		for (plist = vlist->procs; plist != NULL; plist = plist->next) {
			if (!newstyle || plist->arg_num < 2)
				continue; /* old style, or single argument */
			//print_prog_header(plist); XXX
			for (dl = plist->args.decls; dl != NULL;
			     dl = dl->next)
				print_stat(1, &dl->decl, 1);
			print_trailer();
		}
}


static void
emit_union(def)
	definition *def;
{
	declaration *dflt;
	case_list *cl;
	declaration *cs;
	int encode;

	for (encode = 1; encode >= 0; encode--) {
		print_header(def->def_name, encode);

		f_print(fout, "\txs %s v.%s;\n", encode ? "<<" : ">>",
			def->def.un.enum_decl.name);

		f_print(fout, "\tswitch (v.%s) {\n",
			def->def.un.enum_decl.name);

		for (cl = def->def.un.cases; cl != NULL; cl = cl->next) {
			f_print(fout, "\tcase %s:\n", cl->case_name);
			if (cl->contflag == 1) /* a continued case statement */
				continue;
			cs = &cl->case_decl;
			if (!streq(cs->type, "void"))
				print_stat(2, cs, encode);
			f_print(fout, "\t\tbreak;\n");
		}

		dflt = def->def.un.default_decl;
		if (dflt != NULL) {
			f_print(fout, "\tdefault:\n");
			if (!streq(dflt->type, "void"))
				print_stat(2, dflt, encode);
			f_print(fout, "\t\tbreak;\n");
		}

		f_print(fout, "\t}\n");
		print_trailer();
	}
}

static void
emit_struct(def)
	definition *def;
{
	decl_list *dl;

	print_header(def->def_name, 1);
	for (dl = def->def.st.decls; dl != NULL; dl = dl->next)
		print_stat(1, &dl->decl, 1);
	print_trailer();

	print_header(def->def_name, 0);
	for (dl = def->def.st.decls; dl != NULL; dl = dl->next)
		print_stat(1, &dl->decl, 0);
	print_trailer();
}

static void
emit_typedef(def)
	definition *def;
{
	//char *prefix = def->def.ty.old_prefix;
	char *type = def->def.ty.old_type;
	//char *amax = def->def.ty.array_max;
	relation rel = def->def.ty.rel;

	switch (rel) {
#if 0
	case REL_POINTER:
		f_print(fout, "\n");
		f_print(fout, "template SST::XdrStream &operator<<("
				"SST::XdrStream &xs, const SST::XdrPointer<SST::%s> &p);\n",
			type);
		f_print(fout, "template SST::XdrStream &operator>>("
				"SST::XdrStream &xs, SST::XdrPointer<SST::%s> &p);\n",
			type);
		break;
	case REL_OPTION:
		f_print(fout, "\n");
		f_print(fout, "template SST::XdrStream &operator<<("
				"SST::XdrStream &xs, const SST::XdrOption<SST::%s> &o);\n",
			type);
		f_print(fout, "template SST::XdrStream &operator>>("
				"SST::XdrStream &xs, SST::XdrOption<SST::%s> &o);\n",
			type);
		break;
#endif
	default:
		break;
	}
}

char *upcase ();

static void
emit_inline(indent, decl, flag)
int indent;
declaration *decl;
int flag;
{
	switch (decl->rel) {
	case REL_ALIAS :
		emit_single_in_line(indent, decl, flag, REL_ALIAS);
		break;
	case REL_VECTOR :
		tabify(fout, indent);
		f_print(fout, "{\n");
		tabify(fout, indent + 1);
		f_print(fout, "register %s *genp;\n\n", decl->type);
		tabify(fout, indent + 1);
		f_print(fout,
			"for (i = 0, genp = objp->%s;\n", decl->name);
		tabify(fout, indent + 2);
		f_print(fout, "i < %s; i++) {\n", decl->array_max);
		emit_single_in_line(indent + 2, decl, flag, REL_VECTOR);
		tabify(fout, indent + 1);
		f_print(fout, "}\n");
		tabify(fout, indent);
		f_print(fout, "}\n");
		break;
	default:
		break;
	}
}

static void
emit_single_in_line(indent, decl, flag, rel)
int indent;
declaration *decl;
int flag;
relation rel;
{
	char *upp_case;
	int freed = 0;

	tabify(fout, indent);
	if (flag == PUT)
		f_print(fout, "IXDR_PUT_");
	else
		if (rel == REL_ALIAS)
			f_print(fout, "objp->%s = IXDR_GET_", decl->name);
		else
			f_print(fout, "*genp++ = IXDR_GET_");

	upp_case = upcase(decl->type);

	/* hack	 - XX */
	if (strcmp(upp_case, "INT") == 0)
	{
		free(upp_case);
		freed = 1;
		upp_case = "LONG";
	}

	if (strcmp(upp_case, "U_INT") == 0)
	{
		free(upp_case);
		freed = 1;
		upp_case = "U_LONG";
	}
	if (flag == PUT)
		if (rel == REL_ALIAS)
			f_print(fout,
				"%s(buf, objp->%s);\n", upp_case, decl->name);
		else
			f_print(fout, "%s(buf, *genp++);\n", upp_case);

	else
		f_print(fout, "%s(buf);\n", upp_case);
	if (!freed)
		free(upp_case);
}

char *upcase(str)
char *str;
{
	char *ptr, *hptr;

	ptr = (char *)xmalloc(strlen(str)+1);

	hptr = ptr;
	while (*str != '\0')
		*ptr++ = toupper(*str++);

	*ptr = '\0';
	return (hptr);
}
