// See LICENSE file for license details.
#define _XOPEN_SOURCE 500
#define _GNU_SOURCE
#define CMD_LENGTH 500

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <errno.h>
#include <pwd.h>
#include <unistd.h>
#include <dirent.h>
#include <stdarg.h>
#include <sys/stat.h>
#include <X11/Xutil.h>
#include <X11/Xatom.h>

#if HAVE_SHADOW_H
#include <shadow.h>
#endif

#if HAVE_BSD_AUTH
#include <login_cap.h>
#include <bsd_auth.h>
#endif

// Import configuration. 
#include "config.h"

char program_name[6] = "tlock";

char *g_pw = NULL;
int lock_tries = 0;

typedef struct {
  int screen;
  Window root, win;
  Pixmap pmap;
  unsigned long colors[2];
} Lock;

static Lock **locks;
static int nscreens;
static int running = 1;

static void die(const char *errstr, ...) {
  va_list ap;

  va_start(ap, errstr);
  vfprintf(stderr, errstr, ap);
  va_end(ap);
  exit(EXIT_FAILURE);
}

#ifdef __linux__
#include <fcntl.h>

static void dontkillme(void) {
  errno = 0;
  int fd = open("/proc/self/oom_score_adj", O_WRONLY);

  if (fd < 0 && errno == ENOENT)
    return;

  if (fd < 0)
    goto error;

  if (write(fd, "-1000\n", 6) != 6) {
    close(fd);
    goto error;
  }

  if (close(fd) != 0)
    goto error;

  return;

error:
  fprintf(stderr, "cannot disable the OOM killer for this process\n");
  fprintf(stderr, "trying with sudo.\n");

  pid_t pid = getpid();

  char cmd[CMD_LENGTH];

  int r = snprintf(
    cmd,
    CMD_LENGTH,
    "echo -1000 | sudo -n tee /proc/%u/oom_score_adj > /dev/null 2>& 1",
    (unsigned int)pid
  );

  if (r >= 0 && r < CMD_LENGTH)
    system(cmd);
}
#endif

#ifndef HAVE_BSD_AUTH
static const char *gethash(void) {
  const char *hash;
  struct passwd *pw;

  if (g_pw)
    return g_pw;

  // Fetch password entry for current user.
  errno = 0;
  if (!(pw = getpwuid(getuid()))) {
    if (errno)
      die("%s: getpwuid: %s\n", 
          program_name, strerror(errno));
    else
      die("%s: cannot retrieve password entry\n", 
          program_name);
  }

  endpwent();
  hash = pw->pw_passwd;

#if HAVE_SHADOW_H
  if (hash[0] == 'x' && hash[1] == '\0') {
    struct spwd *sp;

    sp = getspnam(getenv("USER"));
    if (!sp)
      die("%s: getspnam: cannot retrieve shadow entry. "
          "make sure to suid or sgid %s.\n",
          program_name, program_name);
    endspent();
    hash = sp->sp_pwdp;
  }
#endif

  // Drop privileges.
  if (geteuid() == 0) {
    if (!(getegid() != pw->pw_gid && 
          setgid(pw->pw_gid) < 0)) {
      if (setuid(pw->pw_uid) < 0)
        die("%s: cannot drop privileges\n", program_name);
    }
  }

  return hash;
}
#endif

#ifdef PASSWD
static char *read_file(char *name) {
  FILE *f = fopen(name, "r");

  if (f == NULL)
    goto error;

  struct stat s;

  if (stat(name, &s) == -1) {
    fclose(f);
    goto error;
  }

  char *buf = malloc(s.st_size);

  if (buf == NULL) {
    fclose(f);
    goto error;
  }

  fread(buf, 1, s.st_size, f);
  fclose(f);

  char *c = buf;
  while (*c) {
    if (*c == '\r' || *c == '\n') {
      *c = '\0';
      break;
    }
    c++;
  }

  return buf;

error:
    fprintf(stderr, "%s: could not open: %s.\n", program_name, name);
    return NULL;
}
#endif

// Disable ALT+SYSRQ and CTRL+ALT+Backspace
// Prevents ALT+SYSRQ+k'ing our process.
static void disable_kill(void) {
#if POWEROFF
  // /usr/bin/tee /proc/sys/kernel/SYSRQ,/usr/bin/tee /proc/SYSRQ-trigger
  // system("echo 1 | sudo -n tee /proc/sys/kernel/SYSRQ > /dev/null");
  // system("echo o | sudo -n tee /proc/SYSRQ-trigger > /dev/null");
  system("echo 0 | sudo -n tee /proc/sys/kernel/SYSRQ > /dev/null 2>& 1 &");

#if DISABLE_ZAP
  // Disable CTRL+ALT+Backspace
  system("setxkbmap -option &");
#endif

#else
  return;
#endif
}

// Poweroff if in danger.
static void poweroff(void) {
#if POWEROFF
  // Needs sudo privileges - alter your /etc/sudoers file:
  // systemd: [username] [hostname] =NOPASSWD: /usr/bin/systemctl poweroff
  // sysvinit: [username] [hostname] =NOPASSWD: /usr/bin/shutdown -h now
  system("sudo -n systemctl poweroff 2> /dev/null");
  system("sudo -n shutdown -h now 2> /dev/null");
#else
  return;
#endif
}

// Take a webcam shot of whoever is tampering with our
// machine.
static int webcam_shot(int async) {
#if WEBCAM_SHOT
  char cmd[CMD_LENGTH];

  int r = snprintf(
    cmd,
    CMD_LENGTH,
    "ffmpeg -y -loglevel quiet -f video4linux2 -i /dev/video0"
    " -frames:v 1 -f image2 %s/%s.jpg%s",
    getenv("HOME"),
    program_name,
    async ? " &" : ""
  );

  if (r < 0 || r >= CMD_LENGTH)
    return 0;

  system(cmd);

  return 1;
#else
  return 0;
#endif
}

static int play_beep(int async) {
#if PLAY_AUDIO
  char cmd[CMD_LENGTH];

  int r = snprintf(
    cmd,
    CMD_LENGTH,
#if AUDIO_FILE
    "aplay %s/%s/%s.wav 2> /dev/null%s",
    getenv("HOME"),
    CONFIG_DIR,
    AUDIO_FILE,
#else
    "play -n synth 0.1 sine 880 vol 0.1 2> /dev/null%s",
#endif
    async ? " &" : ""
  );

  if (r >= 0 && r < CMD_LENGTH)
      system(cmd);

  return 1;
#else
  return 0;
#endif
}

void set_tlock_background(
    Display *display, unsigned int color
    ) {
      int screen;

      for (screen = 0; screen < nscreens; screen++) {
        XSetWindowBackground(
          display,
          locks[screen]->win,
          locks[screen]->colors[color]
        );
        XClearWindow(display, locks[screen]->win);
      }
}

static void
#ifdef HAVE_BSD_AUTH
readpw(Display *display)
#else
readpw(Display *display, const char *pws)
#endif
{
  char buf[32], passwd[256];
  int num, screen;
  unsigned int len = 0;
#if COLORS
  unsigned int llen = 0;
#endif
  KeySym ksym;
  XEvent ev;

  running = 1;

  // The DPMS settings have been removed and you can set
  // them with "xset" or something. This way you can 
  // easily set a customized DPMS timeout.
  while (running && !XNextEvent(display, &ev)) {
    if (ev.type != KeyPress) {
      for (screen = 0; screen < nscreens; screen++)
        XRaiseWindow(display, locks[screen]->win);
      continue;
    }

    buf[0] = 0;

    num = XLookupString(&ev.xkey, buf, sizeof(buf), &ksym, 0);

    if (IsKeypadKey(ksym)) {
      if (ksym == XK_KP_Enter)
        ksym = XK_Return;
      else if (ksym >= XK_KP_0 && ksym <= XK_KP_9)
        ksym = (ksym - XK_KP_0) + XK_0;
    }

    if (IsFunctionKey(ksym)
        || IsKeypadKey(ksym)
        || IsMiscFunctionKey(ksym)
        || IsPFKey(ksym)
        || IsPrivateKeypadKey(ksym)) {
      continue;
    }

    switch(ksym) {
      case XK_Return: {
        passwd[len] = 0;

        if (g_pw) {
          running = strcmp(passwd, g_pw) != 0;
        } else {
#ifdef HAVE_BSD_AUTH
          running = !auth_userokay(getlogin(), NULL, "auth-xlock", passwd);
#else
          char *crypt(const char *key, const char *salt);
          running = strcmp(crypt(passwd, pws), pws) != 0;
#endif
        }

        if (running) {
          XBell(display, 100);
          lock_tries++;

#if MAX_ATTEMPTS
          // Actions to take after too many tries.
          if (MAX_ATTEMPTS && lock_tries > MAX_ATTEMPTS) {
            // Disable ALT+SYSRQ and CTRL+ALT+Backspace
            disable_kill();

            // Take a webcam shot and wait for execution.
            webcam_shot(0);

            // Immediately poweroff.
            poweroff();

#if LOCK_FOREVER
            // If we fail, loop forever.
            for (;;)
              sleep(1);
#endif
          } else {
            // Take an async webcam shot if enabled.
            webcam_shot(1);
          }
#endif

          // Play a sound after at least 1 bad password.
          if (lock_tries > 0) {
            play_beep(0);
          }
        }

        len = 0;

        break;
      }
      case XK_Escape: {
        len = 0;
        break;
      }
      case XK_Delete:
      case XK_BackSpace: {
        if (len)
          len -= 1;
        break;
      }
      case XK_Alt_L:
      case XK_Alt_R:
      case XK_Control_L:
      case XK_Control_R:
      case XK_Meta_L:
      case XK_Meta_R:
      case XK_Super_L:
      case XK_Super_R:
      case XK_F1:
      case XK_F2:
      case XK_F3:
      case XK_F4:
      case XK_F5:
      case XK_F6:
      case XK_F7:
      case XK_F8:
      case XK_F9:
      case XK_F10:
      case XK_F11:
      case XK_F12:
      case XK_F13: {
        // Disable ALT+SYSRQ and CTRL+ALT+Backspace.
        disable_kill();

        // Take a webcam shot of whoever
        // is tampering with our machine.
        webcam_shot(0);

        // Immediately poweroff:
        poweroff();

#if LOCK_FOREVER
            // If we fail, loop forever.
            for (;;)
              sleep(1);
#endif

         break;
      }
      default: {
        if (num && !iscntrl((int)buf[0]) && (len + num < sizeof(passwd))) {
          memcpy(passwd + len, buf, num);
          len += num;
        }
        break;
      }
    }

#if COLORS
    if (llen == 0 && len != 0) {
      set_tlock_background(display, 1);
    } else if (llen != 0 && len == 0) {
      set_tlock_background(display, 0);
    }

    llen = len;
#endif
  }
}

static void unlockscreen(Display *display, Lock *lock) {
  if (display == NULL || lock == NULL)
    return;

  XUngrabPointer(display, CurrentTime);

  XFreeColors(display, DefaultColormap(display, lock->screen), lock->colors, 2, 0);
#if HIDE_CURSOR
  XFreePixmap(display, lock->pmap);
#endif

  XDestroyWindow(display, lock->win);

  free(lock);
}

static Lock *lockscreen(Display *display, int screen) {
  unsigned int len;
  Lock *lock;
  XSetWindowAttributes wa;

  if (display == NULL || screen < 0)
    return NULL;

  lock = malloc(sizeof(Lock));

  if (lock == NULL)
    return NULL;

  lock->screen = screen;

  lock->root = RootWindow(display, lock->screen);

#if TRANSPARENCY
  XVisualInfo vi;
  XMatchVisualInfo(display, DefaultScreen(display), 32, TrueColor, &vi);
  wa.colormap = XCreateColormap(
    display,
    DefaultRootWindow(display),
    vi.visual,
    AllocNone
  );
#endif

  // init
  wa.override_redirect = 1;
#if TRANSPARENCY
  wa.border_pixel = 0;
  wa.background_pixel = 0xaa000000;
#else
  wa.background_pixel = BlackPixel(display, lock->screen);
#endif

#if TRANSPARENCY
  int field = CWOverrideRedirect | CWBackPixel | CWColormap | CWBorderPixel;
  lock->win = XCreateWindow(
    display,
    lock->root,
    0,
    0,
    DisplayWidth(display, lock->screen),
    DisplayHeight(display, lock->screen),
    0,
    vi.depth,
    CopyFromParent,
    vi.visual,
    field,
    &wa
  );
#else
  int field = CWOverrideRedirect | CWBackPixel;
  lock->win = XCreateWindow(
    display,
    lock->root,
    0,
    0,
    DisplayWidth(display, lock->screen),
    DisplayHeight(display, lock->screen),
    0,
    DefaultDepth(display, lock->screen),
    CopyFromParent,
    DefaultVisual(display, lock->screen),
    field,
    &wa
  );
#endif

  Atom name_atom = XA_WM_NAME;
  XTextProperty name_prop = { (unsigned char *)program_name, name_atom, 8, 5 };
  XSetWMName(display, lock->win, &name_prop);

  XClassHint *hint = XAllocClassHint();
  if (hint) {
    hint->res_name = program_name;
    hint->res_class = program_name;
    XSetClassHint(display, lock->win, hint);
    XFree(hint);
  }

  XColor color, dummy;

  char curs[] = {0, 0, 0, 0, 0, 0, 0, 0};
  int cmap = DefaultColormap(display, lock->screen);

#if COLORS
  XAllocNamedColor(display, cmap, ACTIVE_COLOR, &color, &dummy);
  lock->colors[1] = color.pixel;

  XAllocNamedColor(display, cmap, LOCKED_COLOR, &color, &dummy);
  lock->colors[0] = color.pixel;
#endif

  // Cursor.
  Cursor locked_cursor = None;

#if HIDE_CURSOR
  XAllocNamedColor(display, cmap, CURSOR_COLOR, &color, &dummy);
  lock->pmap = XCreateBitmapFromData(display, lock->win, curs, 8, 8);

  locked_cursor = XCreatePixmapCursor(
    display, lock->pmap, lock->pmap, &color, &color, 0, 0);

  XDefineCursor(display, lock->win, locked_cursor);
#endif

  XMapRaised(display, lock->win);

  for (len = 1000; len > 0; len--) {
    int field = ButtonPressMask | ButtonReleaseMask | PointerMotionMask;

    int grab = XGrabPointer(
      display,
      lock->root,
      False,
      field,
      GrabModeAsync,
      GrabModeAsync,
      None,
      locked_cursor,
      CurrentTime
    );

    if (grab == GrabSuccess)
      break;

    usleep(1000);
  }

  if (running && (len > 0)) {
    for (len = 1000; len; len--) {
      int grab = XGrabKeyboard(
        display,
        lock->root,
        True,
        GrabModeAsync,
        GrabModeAsync,
        CurrentTime
      );

      if (grab == GrabSuccess)
        break;

      usleep(1000);
    }
  }

  running &= (len > 0);

  if (!running) {
    unlockscreen(display, lock);
    lock = NULL;
  } else {
    XSelectInput(display, lock->root, SubstructureNotifyMask);
  }

  return lock;
}

static void usage(void) {
  fprintf(stderr, "usage: %s [-v]\n", program_name);
  exit(0);
}

static char *read_pw_file(void) {
#ifndef PASSWD
  return NULL;
#else
  char pw_dir[255], pw_file[256];

  int res = snprintf(
    pw_dir,
    sizeof(pw_dir),
#if USE_HOME_PATH
    "%s/%s",
    getenv("HOME"),
#else
    "%s",
#endif
    CONFIG_DIR
  );

  if (res < 0 || res >= sizeof(pw_dir))
    return NULL;

  DIR* dir = opendir(pw_dir);

  if (dir) { // Directory exists
    closedir(dir);

    if (access(PASSWD, F_OK) == 0) { // Can't access passwd file
      return NULL;
    }
  } else { // Can't access config directory
    printf("%s: error: can't access %s directory\n"
           "warning: defaulting to shadow file.\n",
           program_name, program_name);
    return NULL;
  }

  res = snprintf(
    pw_file,
    sizeof(pw_file),
    "%s/%s",
    pw_dir,
    PASSWD
  );

  if (res < 0 || res >= sizeof(pw_file))
    return NULL;

  return read_file(pw_file);
#endif
}

int main(int argc, char **argv) {
#ifndef HAVE_BSD_AUTH
  const char *pws;
#endif
  Display *display;
  int screen;

#ifdef QUIET_MODE
  freopen("/dev/null", "a", stdout);
  freopen("/dev/null", "a", stderr);
#endif

  if ((argc >= 2) && strcmp(argv[1], "-v") == 0) {
    die("%s-%s\n© 2023 cherry-noize\n", 
        program_name, VERSION);
  } else if (argc != 1) {
    usage();
  }

  g_pw = read_pw_file();

#ifdef __linux__
  dontkillme();
#endif

  if (!g_pw && !getpwuid(getuid()))
    die("%s: no passwd entry found\n", program_name);

#ifndef HAVE_BSD_AUTH
  pws = gethash();
#endif

  display = XOpenDisplay(0);
  if (!display)
    die("%s: cannot open display\n", program_name);

  // Get the number of screens in display "display" and blank
  // them all.
  nscreens = ScreenCount(display);

  errno = 0;
  locks = malloc(sizeof(Lock *) * nscreens);

  if (locks == NULL)
    die("%s: malloc: %s\n", 
        program_name, strerror(errno));

  int nlocks = 0;

  for (screen = 0; screen < nscreens; screen++) {
    locks[screen] = lockscreen(display, screen);
    if (locks[screen] != NULL)
      nlocks++;
  }
      
  set_tlock_background(display, 0);

  XSync(display, False);

  // Did we actually manage to lock something?
  if (nlocks == 0) { // Nothing to protect
    free(locks);
    XCloseDisplay(display);
    return 1;
  }

  // Screen locked. Wait for correct password.
#ifdef HAVE_BSD_AUTH
  readpw(display);
#else
  readpw(display, pws);
#endif

  // Password ok, unlock everything and quit.
  for (screen = 0; screen < nscreens; screen++)
    unlockscreen(display, locks[screen]);

  free(locks);
  XCloseDisplay(display);

  return 0;
}
