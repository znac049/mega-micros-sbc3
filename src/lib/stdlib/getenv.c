/*
MIT License

Copyright (c) 2026 Bob Green

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all
copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
SOFTWARE.
*/

#include <stdlib.h>
#include <string.h>
#include <stddef.h>
#include <errno.h>

/*
 * The environment: "NAME=value" strings. Programs start with an empty
 * one (the monitor has no environment to pass on) and can fill it in
 * with setenv() or putenv().
 */
static char *no_vars[1] = { NULL };
char **environ = no_vars;

static int env_count;           // entries in environ
static int env_room;            // room in it, if it's from malloc()

// Where 'name' is in environ, or -1
static int find_var(const char *name, size_t len) {
    for (int i=0; i<env_count; i++) {
        if ((strncmp(environ[i], name, len) == 0) && (environ[i][len] == '=')) {
            return i;
        }
    }

    return -1;
}

static int valid_name(const char *name) {
    return (name != NULL) && (name[0] != '\0') && (strchr(name, '=') == NULL);
}

char *getenv(const char *name) {
    int i;

    if (!valid_name(name)) {
        return NULL;
    }

    i = find_var(name, strlen(name));

    return (i < 0)?NULL:(environ[i] + strlen(name) + 1);
}

// Make the string 'var' ("NAME=value") part of the environment, as it is
static int put_var(char *var, size_t name_len) {
    int i = find_var(var, name_len);

    if (i >= 0) {
        environ[i] = var;
        return 0;
    }

    if (env_count + 1 >= env_room) {
        int room = (env_room == 0)?16:(env_room * 2);
        char **bigger = malloc(room * sizeof(char *));

        if (bigger == NULL) {
            errno = ENOMEM;
            return -1;
        }

        for (int j=0; j<env_count; j++) {
            bigger[j] = environ[j];
        }

        if (env_room != 0) {
            free(environ);
        }

        environ = bigger;
        env_room = room;
    }

    environ[env_count++] = var;
    environ[env_count] = NULL;

    return 0;
}

int putenv(char *string) {
    char *eq = (string != NULL)?strchr(string, '='):NULL;

    if ((eq == NULL) || (eq == string)) {
        errno = EINVAL;
        return -1;
    }

    return put_var(string, (size_t)(eq - string));
}

int setenv(const char *name, const char *value, int overwrite) {
    size_t name_len, value_len;
    char *var;

    if (!valid_name(name)) {
        errno = EINVAL;
        return -1;
    }

    if (value == NULL) {
        value = "";
    }

    name_len = strlen(name);
    if (!overwrite && (find_var(name, name_len) >= 0)) {
        return 0;
    }

    value_len = strlen(value);
    var = malloc(name_len + value_len + 2);
    if (var == NULL) {
        errno = ENOMEM;
        return -1;
    }

    memcpy(var, name, name_len);
    var[name_len] = '=';
    memcpy(var + name_len + 1, value, value_len + 1);

    // (A string it replaces may be the caller's, from putenv(), so isn't freed)
    if (put_var(var, name_len) != 0) {
        free(var);
        return -1;
    }

    return 0;
}

int unsetenv(const char *name) {
    int i;

    if (!valid_name(name)) {
        errno = EINVAL;
        return -1;
    }

    i = find_var(name, strlen(name));
    if (i >= 0) {
        for (; i<env_count; i++) {
            environ[i] = environ[i + 1];
        }

        env_count--;
    }

    return 0;
}
