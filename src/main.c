#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <stdbool.h>
#include <libgen.h>
#include <string.h>
#include <unistd.h>
#include <stdarg.h>

#define PCRE2_CODE_UNIT_WIDTH 8
#include <pcre2.h>

/*
** Globals
*/

typedef struct
{
	int sketch_num;
	int num_lines;
	char **lines;
	bool has_tupple;
	bool is_native;
	char *sketch_name;
	char *setup_name;
	char *loop_name;
} Sketch;

/*
** Debug output
*/
#define DEBUG 0
#define DEBUG_SKETCH 1

void dbg_printf(const char *fmt, ...)
{
	va_list args;
	va_start(args, fmt);
	vfprintf(stderr, fmt, args);
	va_end(args);
}

void dbg_print_sketch(Sketch **sketches)
{
	int i=0, j=0;
	while (sketches[i] != NULL)
	{
		while(sketches[i]->lines[j] != NULL)
		{
			fprintf(stderr, "%s", sketches[i]->lines[j]);
			j++;
		}
		i++;
	}
}

#define TRACE_SKETCH(x)         \
	do                          \
	{                           \
		if (DEBUG_SKETCH)       \
			dbg_print_sketch x; \
	} while (0)

#define TRACE(x)          \
	do                    \
	{                     \
		if (DEBUG)        \
			dbg_printf x; \
	} while (0)

Sketch **read_sketches(char *file)
{
	FILE *fp;
	ssize_t read;
	size_t len = 0;
	char *line = NULL;
	int line_num = 0;
	int num_sketches = 0;
	Sketch **sketches;

	sketches = malloc(sizeof(Sketch *));
	sketches[num_sketches] = malloc(sizeof(Sketch));
	sketches[num_sketches]->num_lines = 0;
	sketches[num_sketches]->has_tupple = false;
	sketches[num_sketches]->is_native = false;
	sketches[num_sketches]->setup_name = NULL;
	sketches[num_sketches]->loop_name = NULL;
	sketches[num_sketches]->sketch_num = num_sketches;
	sketches[num_sketches]->lines = (char **)malloc(sizeof(char *));

	fp = fopen(file, "r");

	if (fp == NULL)
	{
		TRACE(("no such file %s\n", file));
		exit(EXIT_FAILURE);
	}

	while ((read = getline(&line, &len, fp)) != -1)
	{
		if (strstr(line, "#line 1 "))
		{
			/* Terminate Sketch with a NULL line */
			sketches[num_sketches]->lines = (char **)realloc(sketches[num_sketches]->lines, sizeof(char *) * (line_num + 1));
			sketches[num_sketches]->lines[line_num] = NULL;

			num_sketches++;
			line_num = 0;
			sketches = realloc(sketches, sizeof(Sketch *) * (num_sketches + 1));
			sketches[num_sketches] = malloc(sizeof(Sketch));
			sketches[num_sketches]->num_lines = 0;
			sketches[num_sketches]->has_tupple = false;
			sketches[num_sketches]->is_native = false;
			sketches[num_sketches]->setup_name = NULL;
			sketches[num_sketches]->loop_name = NULL;
			sketches[num_sketches]->sketch_num = num_sketches;
			sketches[num_sketches]->lines = (char **)malloc(sizeof(char *));
			/* 
			 * Get the Sketch name from the path.
			 * A copy of line is made here because basename in Linux
			 * behaves differently than in macOS.
			 */
			char *line_copy = malloc(strlen(line) + 1);
			strcpy(line_copy, line);
			char *name = basename(line_copy);
			char *ext = strrchr(name, '.');
			*ext = '\0';
			sketches[num_sketches]->sketch_name = malloc(strlen(name) + 1);
			memcpy(sketches[num_sketches]->sketch_name, name, strlen(name) + 1);
			free(line_copy);
		}

		sketches[num_sketches]->lines = (char **)realloc(sketches[num_sketches]->lines, sizeof(char *) * (line_num + 1));

		sketches[num_sketches]->lines[line_num] = malloc(read+1);
		sketches[num_sketches]->lines[line_num][read] = '\0';
		strncpy(sketches[num_sketches]->lines[line_num], line, read);
		sketches[num_sketches]->num_lines++;
		free(line);
		line = NULL;

		line_num++;
	}

	/* Terminate Sketch with a NULL line */
	sketches[num_sketches]->lines = (char **)realloc(sketches[num_sketches]->lines, sizeof(char *) * (line_num + 1));
	sketches[num_sketches]->lines[line_num] = NULL;

	num_sketches++;
	/* Add one more element as a NULL terminator */
	sketches = realloc(sketches, sizeof(Sketch *) * (num_sketches + 1));
	sketches[num_sketches] = NULL;

	fclose(fp);

	return sketches;
}

char *find_by_regex(char **lines, const unsigned char *pattern)
{
	int i;
	int namecount;
	int rc;
	int name_entry_size;
	PCRE2_SPTR name_table;
	PCRE2_SIZE erroroffset;
	PCRE2_SIZE *ovector;
	pcre2_code *re;
	pcre2_match_data *match_data;
	int errornumber;
	char *match = NULL;

	re = pcre2_compile(
		pattern,			   /* the pattern */
		PCRE2_ZERO_TERMINATED, /* indicates pattern is zero-terminated */
		0,					   /* default options */
		&errornumber,		   /* for error number */
		&erroroffset,		   /* for error offset */
		NULL);				   /* use default compile context */

	match_data = pcre2_match_data_create_from_pattern(re, NULL);
	ovector = pcre2_get_ovector_pointer(match_data);

	i = 0;

	while (lines[i] != NULL)
	{
		rc = pcre2_match(
			re,					  /* the compiled pattern */
			(PCRE2_SPTR)lines[i], /* the subject string */
			strlen(lines[i]),	  /* the length of the subject */
			0,					  /* start at offset 0 in the subject */
			0,					  /* default options */
			match_data,			  /* block for storing the result */
			NULL);				  /* use default match context */

		if (rc < 0)
		{
			i++;
			continue;
		}

		// lines[i][strcspn(lines[i], "\r\n")] = 0;
		TRACE(("Match on line #%d: => %s", i, lines[i]));

		ovector = pcre2_get_ovector_pointer(match_data);

		(void)pcre2_pattern_info(
			re,					  /* the compiled pattern */
			PCRE2_INFO_NAMECOUNT, /* get the number of named substrings */
			&namecount);		  /* where to put the answer */

		if (namecount <= 0)
		{
			i++;
			continue;
		}

		PCRE2_SPTR tabptr;

		/*
			 * Before we can access the substrings, we must extract the table for
			 * translating names to numbers, and the size of each entry in the table.
			 */

		(void)pcre2_pattern_info(
			re,					  /* the compiled pattern */
			PCRE2_INFO_NAMETABLE, /* address of the table */
			&name_table);		  /* where to put the answer */

		(void)pcre2_pattern_info(
			re,						  /* the compiled pattern */
			PCRE2_INFO_NAMEENTRYSIZE, /* size of each entry in the table */
			&name_entry_size);		  /* where to put the answer */

		/*
			 * Now we can scan the table and, for each entry, print the number, the name,
			 * and the substring itself. In the 8-bit library the number is held in two
			 * bytes, most significant first.
			 */

		tabptr = name_table;
		int n = (tabptr[0] << 8) | tabptr[1];
		int len = (int)((ovector[2 * n + 1] - ovector[2 * n]));
		match = malloc(len + 1);
		match[len] = '\0';
		strncpy(match, lines[i] + ovector[2 * n], len);
		TRACE(("%s\n", match));
		break;
	}

	pcre2_match_data_free(match_data); /* Release memory used for the match */
	pcre2_code_free(re);
	return match;
}

void process_sketch(Sketch *sketch)
{
	PCRE2_SPTR setup_pattern = (PCRE2_SPTR) "(?P<func>([sS][eE][tT][uU][pP][a-zA-Z0-9_]*)|([a-zA-Z0-9_]+[sS][eE][tT][uU][pP]))(?:\\(\\h*(?:void)*\\h*\\)\\h*)\\h*[^\\h*;]";
	PCRE2_SPTR loop_pattern = (PCRE2_SPTR) "(?P<func>([lL][oO]{2}[pP][a-zA-Z0-9_]*)|([a-zA-Z0-9_]+[lL][oO]{2}[pP]))(?:\\(\\h*(?:void)*\\h*\\)\\h*)\\h*[^\\h*;]";
	sketch->setup_name = find_by_regex(sketch->lines, setup_pattern);
	sketch->loop_name = find_by_regex(sketch->lines, loop_pattern);

	if (sketch->loop_name && sketch->setup_name)
	{
		sketch->has_tupple = true;
		if (!strcmp(sketch->loop_name, "loop") && !strcmp(sketch->setup_name, "setup"))
		{
			sketch->is_native = true;
		}
	}
}

bool write_out_cpp(Sketch **sketches, char *location)
{
	FILE *fp;
	fp = fopen(location, "w");
	int j, i;

	char *undef = "#undef loop\n#undef setup\n";

	if (fp == NULL)
	{
		exit(EXIT_FAILURE);
	}

	i = 0;
	while (sketches[i] != NULL)
	{
		j = 0;
		if (sketches[i]->has_tupple && sketches[i]->is_native)
		{
			fwrite(undef, 1, strlen(undef), fp);
			fprintf(fp, "#define setup setup%s\n", sketches[i]->sketch_name);
			fprintf(fp, "#define loop loop%s\n", sketches[i]->sketch_name);
		}

		while (sketches[i]->lines[j] != NULL)
		{
			fwrite(sketches[i]->lines[j], 1, strlen(sketches[i]->lines[j]), fp);
			j++;
		}
		i++;
	}

	fclose(fp);
	return true;
}

void write_main_cpp(Sketch **sketches, char *output, char *template)
{
	FILE *in, *out;
	ssize_t read;
	size_t len = 0;
	int num_sketches = 0;
	int i = 0, j = 0;
	char *line = NULL;
	char *magic = "769d20fcd7a0eedaf64270f591438b01";
	char *func_ptrs_prologue = "\nvoid (*func_ptr[NUM_SKETCHES][2])(void) = {\n";
	char *func_ptrs_epilogue = "\n};\n\n";
	char *task_name_array_prologue = "const char *taskNames[] = {\n";
	char *task_name_array_epilogue = "\n};\n";

	in = fopen(template, "r");
	out = fopen(output, "w");

	if (in == NULL || out == NULL)
	{
		TRACE(("Failed to open input or output file\n"));
		exit(EXIT_FAILURE);
	}

	/* 
	 * Calculate number of setup/loop tuples.
	 */
	while (sketches[i])
	{
		if (sketches[i]->has_tupple)
		{
			num_sketches++;
		}

		i++;
	}

	while ((read = getline(&line, &len, in)) != -1)
	{
		/* read / write the file up to the magic insertion point. */
		fwrite(line, 1, strlen(line), out);
		if (strstr(line, magic))
		{
			TRACE(("Found magic insertion point\n"));
			break;
		}
	}

	fprintf(out, "#define NUM_SKETCHES %d\n\n", num_sketches);

	/*
	 * Write out function declerations.
	 */
	i = 0;
	while (sketches[i])
	{
		if (sketches[i]->has_tupple)
		{
			if (sketches[i]->is_native)
			{
				/* Mangle name */
				fprintf(out, "extern void setup%s();\n", sketches[i]->sketch_name);
				fprintf(out, "extern void loop%s();\n", sketches[i]->sketch_name);
			}
			else
			{
				fprintf(out, "extern void %s();\n", sketches[i]->setup_name);
				fprintf(out, "extern void %s();\n", sketches[i]->loop_name);
			}
		}
		i++;
	}

	/*
	 * Write out function pointer array.
	 */
	fwrite(func_ptrs_prologue, 1, strlen(func_ptrs_prologue), out);

	i = 0;
	j = 0;

	while (sketches[i])
	{
		if (sketches[i]->has_tupple)
		{
			j++;
			if (sketches[i]->is_native)
			{
				/* Mangle name */
				fprintf(out, "\t{setup%s, loop%s}", sketches[i]->sketch_name, sketches[i]->sketch_name);
			}
			else
			{
				fprintf(out, "\t{setup%s, loop%s}", sketches[i]->setup_name, sketches[i]->loop_name);
			}

			if (j < num_sketches)
			{
				fprintf(out, ",\n");
			}
		}
		i++;
	}

	fwrite(func_ptrs_epilogue, 1, strlen(func_ptrs_epilogue), out);

	/* 
	 * Write out task name array.
	 */
	i = 0;
	j = 0;
	fwrite(task_name_array_prologue, 1, strlen(task_name_array_prologue), out);
	while (sketches[i])
	{
		if (sketches[i]->has_tupple)
		{
			j++;
			if (sketches[i]->is_native)
			{
				/* Mangle name. */
				fprintf(out, "\t\"loop%s\"", sketches[i]->sketch_name);
			}
			else
			{
				fprintf(out, "\t\"%s\"", sketches[i]->loop_name);
			}

			if (j < num_sketches)
			{
				fprintf(out, ",\n");
			}
		}
		i++;
	}
	fwrite(task_name_array_epilogue, 1, strlen(task_name_array_epilogue), out);

	/* 
	 * Write out the rest of the file.
	 */
	while ((read = getline(&line, &len, in)) != -1)
	{
		fwrite(line, 1, strlen(line), out);
	}
}

void usage(char *name)
{
	fprintf(stdout, "Usage: %s -s [file...] -t [template...]\n", name);
}

int main(int argc, char *argv[])
{
	int i, opt;
	Sketch **sketches;
	char *sketch_file = NULL;
	char *template = NULL;
	char *main_file = NULL;
	bool opt_error = false;
	FILE *fp;

	while ((opt = getopt(argc, argv, "s:t:o:")) != -1)
	{
		switch (opt)
		{
		case 's':
			sketch_file = optarg;
			break;
		case 't':
			template = optarg;
			break;
		case 'o':
			main_file = optarg;
			break;
		default:
			usage(argv[0]);
			exit(EXIT_FAILURE);
		}
	}

	if (!sketch_file)
	{
		fprintf(stderr, "-s is mandetory!\n");
		opt_error = true;
	}

	if (!template)
	{
		fprintf(stderr, "-t is mandetory!\n");
		opt_error = true;
	}

	if (!main_file)
	{
		fprintf(stderr, "-o is mandetory!\n");
		opt_error = true;
	}

	if (opt_error)
	{
		usage(argv[0]);
		exit(EXIT_FAILURE);
	}

	if (access(sketch_file, R_OK) < 0)
	{
		fprintf(stderr, "%s no such file\n", sketch_file);
		opt_error = true;
	}

	if ((fp = fopen(main_file, "w")) == NULL)
	{
		fprintf(stderr, "%s no such directory\n", main_file);
		opt_error = true;
	}
	else
	{
		fclose(fp);
	}

	if (access(template, R_OK) < 0)
	{
		fprintf(stderr, "%s no such file\n", template);
		opt_error = true;
	}

	if (opt_error)
	{
		usage(argv[0]);
		exit(EXIT_FAILURE);
	}

	TRACE(("========== Reading sketchfile =========\n"));
	sketches = read_sketches(sketch_file);

	i = 0;
	while (sketches[i] != NULL)
	{
		TRACE(("========= Processing Sketch %d =========\n", i));
		TRACE(("Number of lines: %d\n", sketches[i]->num_lines));

		process_sketch(sketches[i]);

		if (sketches[i]->has_tupple)
		{
			TRACE(("has tupple\n"));
		}
		i++;
	}

	write_out_cpp(sketches, sketch_file);
	write_main_cpp(sketches, main_file, template);
}