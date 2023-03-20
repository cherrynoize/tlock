// Shutdown after too many tries.
// Needs sudo privileges - add to /etc/sudoers:
// [username] [hostname] =NOPASSWD: /usr/bin/tee /proc/sys/kernel/SYSRQ
//#define POWEROFF 1

// Disable CTRL+ALT+Backspace during shutdown. 
//#define DISABLE_ZAP 1

// Maximum number of password attempts.
//#define MAX_ATTEMPTS 5

// Take a snapshot before shutdown.
//#define WEBCAM_SHOT 1

// Display screen colors.
#define COLORS 1

// Lock screen transparency.
#define TRANSPARENCY 1

// Hide cursor while screen is locked.
#define HIDE_CURSOR 1

// Lock in an endless loop if poweroff failure.
#define LOCK_FOREVER 0

// Play sound alarm on bad password entry.
// (By default uses `play` from `sox` package.)
#define PLAY_AUDIO 0

// Custom alarm sound file.
// Should be placed in tlock config path.
//#define AUDIO_FILE ""

// Initial screen color.
#define INIT_COLOR   "#836880"

// Screen color while typing.
#define ACTIVE_COLOR "#496583"

// Screen color after a failed attempt.
#define LOCKED_COLOR INIT_COLOR

// Screen color after too many failed attempts.
#define ALERT_COLOR  "#c82638"

// Cursor color.
#define CURSOR_COLOR "#191919"

// Config directory (relative to $HOME).
#define CONFIG_DIR ".config/tlock"

// Name of the passwd file.
#define PASSWD "passwd" 
