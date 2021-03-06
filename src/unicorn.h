#ifndef __INC_UNICORN_H__
#define __INC_UNICORN_H__


#include <stddef.h>
#include <mowgli.h>


/* src/message.c */

#define IRC_MESSAGE_SOURCE_NONE   0
#define IRC_MESSAGE_SOURCE_SERVER 1
#define IRC_MESSAGE_SOURCE_USER   2

struct irc_message {
	/* 512-byte character buffer */
	char buffer[513];

	/* potential source */
	union irc_message_source {
		unsigned type;

		struct {
			unsigned type;
			char *name;
		} server;

		struct {
			unsigned type;
			char *nick;
			char *ident;
			char *host;
		} user;
	} source;

        /* mandatory command; numerics remain ASCII-encoded */
        char *command;

        /* arguments */
        mowgli_list_t args;
};
typedef union irc_message_source irc_message_source_t;
typedef struct irc_message irc_message_t;

extern irc_message_t *irc_message_create();
extern int irc_message_destroy(irc_message_t *msg);
extern int irc_message_reset(irc_message_t *msg);

extern int irc_message_source_parse(irc_message_source_t *source, char *spec);
extern int irc_message_source_format(irc_message_source_t *source, mowgli_string_t *str);

extern int irc_message_parse_buffer(irc_message_t *msg);
extern int irc_message_parse(irc_message_t *msg, const char *spec);
extern int irc_message_format(irc_message_t *msg, mowgli_string_t *str);


/* src/casemap.c */

#define IRC_CASEMAPPING_UNKNOWN 0
#define IRC_CASEMAPPING_ASCII 1
#define IRC_CASEMAPPING_RFC1459 2
#define IRC_CASEMAPPING_STRICT_RFC1459 3

extern int irc_casecmp(int casemapping, const char *a, const char *b);

extern void irc_casemap_null(char *s);
extern void irc_casemap_ascii(char *s);
extern void irc_casemap_rfc1459(char *s);
extern void irc_casemap_strict_rfc1459(char *s);

extern void (*irc_casemap_fn(int casemapping))(char*);


/* src/isupport.c */

struct irc_isupport {
        int casemapping;

        struct {
                mowgli_string_t *list;
                mowgli_string_t *arg_always;
                mowgli_string_t *arg_onset;
                mowgli_string_t *noarg;
        } chanmodes;

        mowgli_string_t *chantypes;

        int modes;

        int nicklen;

        struct {
                mowgli_string_t *mode;
                mowgli_string_t *prefix;
        } prefix;

	mowgli_patricia_t *values;
};
typedef struct irc_isupport irc_isupport_t;

extern irc_isupport_t *irc_isupport_create(void);
extern int irc_isupport_destroy(irc_isupport_t *isupport);

// Reset ISUPPORT to IRC defaults, useful on 001
extern int irc_isupport_reset(irc_isupport_t *isupport);

// This function strncpy's to non-NULL string types
// The args in msg are not left intact
extern int irc_isupport_parse(irc_isupport_t *isupport, irc_message_t *msg);

extern char *irc_isupport_get_prefix_mode(irc_isupport_t *isupport);
extern char *irc_isupport_get_prefix_char(irc_isupport_t *isupport);
extern int irc_isupport_get_casemapping(irc_isupport_t *isupport);


/* src/mode.c */

struct irc_prefix {
	// While theoretically you could have more than 32 prefixes,
	// in practice you typically see no more than 5.
	unsigned long bv;

	// if this is NULL, the standard PREFIX=(ov)@+ applies
	irc_isupport_t *isupport;
};
typedef struct irc_prefix irc_prefix_t;

extern irc_prefix_t *irc_prefix_create(irc_isupport_t *isupport);
extern int irc_prefix_destroy(irc_prefix_t *pfx);

extern int irc_prefix_set(irc_prefix_t *pfx, char mode);
extern int irc_prefix_clear(irc_prefix_t *pfx, char mode);

// will return ASCII space if no prefixes are set
extern char irc_prefix_char(irc_prefix_t *pfx);


/* src/hook.c */

// NOTE: hooks are case-insensitive

typedef int (irc_hook_cb_t)(int parc, const char *parv[], void *ctx);

struct irc_hook_table {
	mowgli_patricia_t *hooks;
};
typedef struct irc_hook irc_hook_t;
typedef struct irc_hook_def irc_hook_def_t;
typedef struct irc_hook_table irc_hook_table_t;

extern irc_hook_table_t *irc_hook_table_create();
extern int irc_hook_table_destroy(irc_hook_table_t *table);

// Callbacks are called in the order they are added and are case-insensitive
extern int irc_hook_add(irc_hook_table_t *table, const char *hook, irc_hook_cb_t *cb);
extern int irc_hook_del(irc_hook_table_t *table, const char *hook, irc_hook_cb_t *cb);

// If a callback returns nonzero, this exits early
extern int irc_hook_call(irc_hook_table_t *table, const char *hook, int parc, const char *parv[], void *ctx);

// This function will take a message and call a particular hook based on
// a simple transformation. The hook to be called is simply the message
// command. The first argument is the sender's nick (or "" if there is
// no sender), and the following arguments are simply the arguments to
// the message.
//
// Examples:
//   ":aji!alex@asu.edu JOIN #lobby"
//       calls "JOIN" with arguments "aji", "#lobby"
//   "PING :t4.general.asu.edu"
//       calls "PING" with arguments "", "t4.general.asu.edu"
//   ":aji!alex@asu.edu PRIVMSG #lobby :hi"
//       calls "PRIVMSG" with arguments "aji", "#lobby", "hi"
extern int irc_hook_simple_dispatch(irc_hook_table_t *table, irc_message_t *msg, void *ctx);

// This function behaves similarly to irc_hook_simple_dispatch, except
// instead of parv[1] being the first argument, parv[3] is the first
// argument, and parv[0], parv[1], and parv[2] are the nickname, ident,
// and host of the source, respectively. If the source is a server,
// parv[0] and parv[1] will be empty strings and parv[2] will be the
// server name. If there is no source, then all of parv[0], parv[1],
// and parv[2] will be empty.
//
// Examples:
//   ":aji!alex@asu.edu JOIN #lobby"
//       calls "JOIN" with arguments "aji", "alex", "asu.edu", "#lobby"
//   "PING :t4.general.asu.edu"
//       calls "PING" with arguments "", "", "", "t4.general.asu.edu"
//   ":t4.general.asu.edu NOTICE :*** Notice -- Looking up your hostname"
//       calls "NOTICE" with arguments "", "", "t4.general.asu.edu",
//       "*** Notice -- Looking up your hostname"
extern int irc_hook_ext_dispatch(irc_hook_table_t *table, irc_message_t *msg, void *ctx);

#endif
