#include <pcre.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define OVECCOUNT 999

// This is our Stanza function to add matches
void append_matches_found(char *match);

// This is our Stanza function to add named groups and their matches
void append_named_matches_found(char *name, char *match);

// And this is our call to PCRE
void pcre_search(char *pattern, char *subject, int find_all)
{
	pcre *re;
	const char *error;
	char **original;
	unsigned char *name_table;
	unsigned int option_bits;
	int erroffset;
	int crlf_is_newline;
	int namecount;
	int name_entry_size;
	int ovector[OVECCOUNT];
	int subject_length = (int)strlen(subject);
	int rc, i;
	int utf8;

	// Compile the regex
	re = pcre_compile(
  	pattern,              // the pattern
  	0,                    // default options
  	&error,               // for error message
  	&erroffset,           // for error offset
  	NULL);                // use default character tables

	// Compilation failed: print the error message and exit
	if (re == NULL)
  {
  	printf("PCRE compilation failed at offset %d: %s\n", erroffset, error);
  	return;
	}

	rc = pcre_exec(
  	re,                   // the compiled pattern
  	NULL,                 // pcre_study might go here
  	subject,              // the subject string
  	subject_length,       // the length of the subject
  	0,                    // start at offset 0 in the subject
  	0,                    // default options
  	ovector,              // output vector for substring information
  	OVECCOUNT);           // number of elements in the output vector

	// Matching failed!
	if (rc < 0)
  {
  	switch(rc)
    {
    	case PCRE_ERROR_NOMATCH: printf("No match\n"); break;
    	default: printf("Matching error %d\n", rc); break;
    }
  	pcre_free(re);
  	return;
  }

	// The output vector wasn't big enough
	if (rc == 0)
  {
  	rc = OVECCOUNT/3;
  	printf("Can only capture a maximum of %d substrings\n", rc - 1);
  }

	for (i = 0; i < rc; i++)
  {
  	char *substring_start = subject + ovector[2*i];
  	int substring_length = ovector[2*i+1] - ovector[2*i];
		char *match = (char*)malloc(substring_length);
		strncpy(match, substring_start, substring_length);
		append_matches_found(match);
		free(match);
  }

	// We find named substrings now
	(void)pcre_fullinfo(
  	re,                   // the compiled pattern
  	NULL,                 // pcre_study might go here
  	PCRE_INFO_NAMECOUNT,  // number of named substrings
  	&namecount);          // where to put the answer

	if (namecount <= 0)
	{}
	else
  {
		unsigned char *tabptr;

		/* We extract table-names and the size required. Note that size if based on
		the largest name in the table */
  	(void)pcre_fullinfo(
    	re,                       // the compiled pattern
    	NULL,                     // pcre_study might go here
    	PCRE_INFO_NAMETABLE,      // address of the table
    	&name_table);             // where to put the answer

  	(void)pcre_fullinfo(
    	re,                       // the compiled pattern
    	NULL,                     // pcre_study might go here
    	PCRE_INFO_NAMEENTRYSIZE,  // size of each entry in the table
    	&name_entry_size);        // where to put the answer

  	/* Now we can scan the table and, for each entry, print the number, the name,
  	and the substring itself. */
  	tabptr = name_table;
  	for (i = 0; i < namecount; i++)
    {
    	int n = (tabptr[0] << 8) | tabptr[1];
			char *name = (char*)malloc(name_entry_size);
			strncpy(name, tabptr + 2, name_entry_size - 3);
			char *match = (char*)malloc(ovector[2*n+1] - ovector[2*n]);
			strncpy(match, subject + ovector[2*n], ovector[2*n+1] - ovector[2*n]);
			append_named_matches_found(name, match);
    	tabptr += name_entry_size;
			free(name);
			free(match);
    }
  }

	// Keep the option for 'find all' here incase we add it in the future
	if (!find_all)
  {
  	pcre_free(re);
  	return;
  }

	/* Before running the loop, check for UTF-8 and whether CRLF is a valid newline
	sequence. First, find the options with which the regex was compiled; extract
	the UTF-8 state, and mask off all but the newline options. */
	(void)pcre_fullinfo(re, NULL, PCRE_INFO_OPTIONS, &option_bits);
	utf8 = option_bits & PCRE_UTF8;
	option_bits &= PCRE_NEWLINE_CR|PCRE_NEWLINE_LF|PCRE_NEWLINE_CRLF|
               PCRE_NEWLINE_ANY|PCRE_NEWLINE_ANYCRLF;

	/* If no newline options were set, find the default newline convention from the
	build configuration. */
	if (option_bits == 0)
  {
  	int d;
  	(void)pcre_config(PCRE_CONFIG_NEWLINE, &d);
  	/* Note that these values are always the ASCII ones, even in
  	EBCDIC environments. CR = 13, NL = 10. */
  	option_bits = (d == 13)? PCRE_NEWLINE_CR :
          				(d == 10)? PCRE_NEWLINE_LF :
      						(d == (13<<8 | 10))? PCRE_NEWLINE_CRLF :
          				(d == -2)? PCRE_NEWLINE_ANYCRLF :
          				(d == -1)? PCRE_NEWLINE_ANY : 0;
  }

	// See if CRLF is a valid newline sequence.
	crlf_is_newline = option_bits == PCRE_NEWLINE_ANY ||
     								option_bits == PCRE_NEWLINE_CRLF ||
     								option_bits == PCRE_NEWLINE_ANYCRLF;

	// Loop for second and subsequent matches
	for (;;)
  {
  	int options = 0;                 /* Normally no options */
  	int start_offset = ovector[1];   /* Start at end of previous match */

  	/* If the previous match was for an empty string, we are finished if we are
  	at the end of the subject. Otherwise, arrange to run another match at the
  	same point to see if a non-empty match can be found. */
  	if (ovector[0] == ovector[1])
    {
    	if (ovector[0] == subject_length) break;
    	options = PCRE_NOTEMPTY_ATSTART | PCRE_ANCHORED;
    }
  	// Run the next matching operation

  	rc = pcre_exec(
    	re,                   // the compiled pattern
    	NULL,                 // pcre_study
    	subject,              // the subject string
    	subject_length,       // the length of the subject
    	start_offset,         // starting offset in the subject
    	options,              // options
    	ovector,              // output vector for substring information
    	OVECCOUNT);           // number of elements in the output vector

  	/* This time, a result of NOMATCH isn't an error. If the value in "options"
  	is zero, it just means we have found all possible matches, so the loop ends.
  	Otherwise, it means we have failed to find a non-empty-string match at a
  	point where there was a previous empty-string match. In this case, we do what
  	Perl does: advance the matching position by one character, and continue. We
  	do this by setting the "end of previous match" offset, because that is picked
  	up at the top of the loop as the point at which to start again.

  	There are two complications: (a) When CRLF is a valid newline sequence, and
  	the current position is just before it, advance by an extra byte. (b)
  	Otherwise we must ensure that we skip an entire UTF-8 character if we are in
  	UTF-8 mode. */

  	if (rc == PCRE_ERROR_NOMATCH)
    {
    	if (options == 0) break;
    	ovector[1] = start_offset + 1;
    	if (crlf_is_newline &&
        start_offset < subject_length - 1 &&
        subject[start_offset] == '\r' &&
        subject[start_offset + 1] == '\n')
      		ovector[1] += 1;
    	else if (utf8)
      {
      	while (ovector[1] < subject_length)
        {
        	if ((subject[ovector[1]] & 0xc0) != 0x80) break;
        	ovector[1] += 1;
        }
      }
    	continue;    // Loop once again
    }

  	// Other matching errors are not recoverable.
  	if (rc < 0)
    {
    	printf("Matching error %d\n", rc);
    	pcre_free(re);
			return;
    }

		// The match succeeded, but the output vector wasn't big enough.
  	if (rc == 0)
    {
    	rc = OVECCOUNT/3;
    	printf("Can only capture a maximum of %d substrings\n", rc - 1);
    }

  	/* As before, show substrings stored in the output vector by number, and then
  	also any named substrings. */
  	for (i = 0; i < rc; i++)
    {
			char *substring_start = subject + ovector[2*i];
	  	int substring_length = ovector[2*i+1] - ovector[2*i];
			char *match = (char*)malloc(substring_length);
			strcpy(match, substring_start);
			append_matches_found(match);
			free(match);
    }

  	if (namecount <= 0)
		{}
		else
    {
    	unsigned char *tabptr = name_table;
    	for (i = 0; i < namecount; i++)
      {
				int n = (tabptr[0] << 8) | tabptr[1];
				char *name = (char*)malloc(name_entry_size);
				strncpy(name, tabptr + 2, name_entry_size - 3);
				char *match = (char*)malloc(ovector[2*n+1] - ovector[2*n]);
				strncpy(match, subject + ovector[2*n], ovector[2*n+1] - ovector[2*n]);
				append_named_matches_found(name, match);
	    	tabptr += name_entry_size;
				free(name);
				free(match);
      }
    }
  }      // End of loop to find second and subsequent matches

	pcre_free(re);
	return;
}
