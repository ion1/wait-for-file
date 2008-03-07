/* wait-for-file: Wait for a file to appear and exit
 * Copyright © 2008 Johan Kiviniemi <devel@johan.kiviniemi.name>
 *
 * Permission to use, copy, modify, and/or distribute this software for any
 * purpose with or without fee is hereby granted, provided that the above
 * copyright notice and this permission notice appear in all copies.
 *
 * THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL WARRANTIES
 * WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF
 * MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR
 * ANY SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES
 * WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN
 * ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF
 * OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
 */

#include <assert.h>
#include <errno.h>
#include <error.h>
#include <libgen.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/inotify.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>

#define INO_BUF_SIZE 2048

static int ino_fd = 0;

static char *watch_path = NULL,  // full path
            *watch_dir  = NULL,  // dirname
            *watch_file = NULL;  // basename

static void
usage (const char *progname, const int retval)
{
  fprintf (stderr, "USAGE: %s <path>\n", progname);

  if (retval != 0)
    exit (retval);
}

static void
split_path (const char *path)
{
  char *copy;

  copy = strdup (path);
  assert (copy != NULL);
  watch_dir = strdup (dirname (copy));
  assert (watch_dir != NULL);
  free (copy);

  copy = strdup (path);
  assert (copy != NULL);
  watch_file = strdup (basename (copy));
  assert (watch_file != NULL);
  free (copy);

  watch_path = strdup (path);
  assert (watch_path != NULL);
}

static void
free_path (void)
{
  free (watch_path);
  free (watch_dir);
  free (watch_file);
  watch_path = watch_dir = watch_file = NULL;
}

static void
init_inotify (void)
{
  ino_fd = inotify_init ();
  if (ino_fd < 0)
    error_at_line (1, errno, __FILE__, __LINE__, "Failed to init inotify");

  int wd = inotify_add_watch (ino_fd, watch_dir, IN_CREATE);
  if (wd < 0)
    error_at_line (1, errno, __FILE__, __LINE__,
                   "Failed to add a watch for %s", watch_dir);
}

static void
deinit_inotify (void)
{
  close (ino_fd);
}

static void
loop_inotify (void)
{
  int keep_going = 1;

  union {
    unsigned char        chars[INO_BUF_SIZE];
    struct inotify_event ev;
  } buf;

  while (keep_going) {
    if (read (ino_fd, &buf.chars, INO_BUF_SIZE) < 1) {
      error_at_line (1, 0, __FILE__, __LINE__,
                     "Inotify read buffer too small: %u", INO_BUF_SIZE);
    }

    if (buf.ev.len && buf.ev.mask == IN_CREATE &&
        ! strcmp (buf.ev.name, watch_file)) {
      keep_going = 0;
    }
  }
}

static int
exists (const char *filename)
{
  struct stat buf;
  return stat (filename, &buf) == 0;
}

int
main (int argc, char **argv)
{
  if (argc != 2)
    usage (argv[0], 1);

  split_path (argv[1]);
  init_inotify ();

  if (! exists (watch_path))
    loop_inotify ();

  deinit_inotify ();
  free_path ();

  return 0;
}

// vim:set et sw=2 sts=2:
