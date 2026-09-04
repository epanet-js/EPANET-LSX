#include <ctype.h>
#include <stdlib.h>
#include <string.h>

#include "inp-parser.h"
#include "file-io.h"

static void trimForSection(const char *line, size_t raw_len, const char **out, size_t *out_len) {
  size_t start = 0;
  while (start < raw_len && isspace((unsigned char)line[start])) start++;

  size_t end = raw_len;
  while (end > start && isspace((unsigned char)line[end - 1])) end--;

  for (size_t i = start; i < end; i++) {
    if (line[i] == ';') {
      end = i;
      break;
    }
  }
  while (end > start && isspace((unsigned char)line[end - 1])) end--;

  *out = line + start;
  *out_len = end - start;
}

static int equalsIgnoreCase(const char *token, size_t len, const char *name) {
  if (len != strlen(name)) return 0;
  for (size_t i = 0; i < len; i++) {
    if (tolower((unsigned char)token[i]) != tolower((unsigned char)name[i])) {
      return 0;
    }
  }
  return 1;
}

static int isSectionHeader(const char *token, size_t len) {
  if (len < 2 || token[0] != '[' || token[len - 1] != ']') return 0;
  for (size_t i = 0; i < len; i++) {
    if (isspace((unsigned char)token[i])) return 0;
  }
  return 1;
}

static char *extractScript(const char *inp_text) {
  if (inp_text == NULL) return NULL;

  char *script = NULL;
  size_t script_len = 0;
  int in_script = 0;

  const char *p = inp_text;
  while (*p != '\0') {
    const char *eol = strchr(p, '\n');
    size_t raw_len = eol ? (size_t)(eol - p) : strlen(p);

    size_t content_len = raw_len;
    if (content_len > 0 && p[content_len - 1] == '\r') content_len--;

    const char *token;
    size_t token_len;
    trimForSection(p, raw_len, &token, &token_len);

    if (!in_script) {
      if (equalsIgnoreCase(token, token_len, "[SCRIPT]")) {
        in_script = 1;
        script = malloc(1);
        if (script == NULL) return NULL;
        script[0] = '\0';
        script_len = 0;
      }
    } else if (isSectionHeader(token, token_len)) {
      break;
    } else {
      char *grown = realloc(script, script_len + content_len + 2);
      if (grown == NULL) {
        free(script);
        return NULL;
      }
      script = grown;
      memcpy(script + script_len, p, content_len);
      script_len += content_len;
      script[script_len++] = '\n';
      script[script_len] = '\0';
    }

    if (eol == NULL) break;
    p = eol + 1;
  }

  return script;
}

char *InpParser_ReadScript(const char *inp_path) {
  char *text = FileIO_ReadText(inp_path);
  if (text == NULL) return NULL;

  char *script = extractScript(text);
  free(text);
  return script;
}
