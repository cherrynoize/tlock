# tlock | tightened simple screen locker

This is a fork of
[a fork of *slock*](https://github.com/chjj/slock)
from which we take inspiration for some sound security 
options but then actually take it back towards a more 
simplicistic approach (more like original *slock*'s) but 
with some added configuration capabilities.

## Screenshots

### Dual kawase blur
![tlock screenshot](https://i.imgur.com/toNopwy.png "tlock with kawase blur")
![tlock screenshot](https://i.imgur.com/9qGTfkM.png "tlock with kawase blur")
### Text impression
![tlock screenshot](https://i.imgur.com/goweCWJ.png "tlock with text message")
### Transparent window
![tlock screenshot](https://i.imgur.com/o7CCIgq.png "tlock with transparency option")
### Opaque
![tlock screenshot](https://i.imgur.com/kuvWYrH.png "tlock with no transparency")

## Configuration

All configuration is done in the `config.h` file.

To get started just make a copy of `config.h.def` into
`config.h`, which you can then edit before compiling.

    cd tlock
    cp config.h.def config.h

### Colors

Colors are fully customizable from the config file.

You can choose colors for the initial lock screen, as
well as for the screen while typing the password, after a bad
password attempt and for the cursor.

### Text

You can add a custom text message through the *PRINT_MSG*
value in `config.h`. *FONT_COLOR* is an #RRGGBB hex value
and *FONT_NAME* is an [XLFD font name](https://www.x.org/releases/X11R7.6/doc/xorg-docs/specs/XLFD/xlfd.html)
which sucks and you're never going to understand it fully. 

### Transparency

You can optionally enable transparency on your lock 
screen. In the config, just set TRANSPARENCY to 1.

> *Note: if the screen just goes black or if this is 
> otherwise not working as expected, you need to ensure
> that you are running a composite manager.*

To hide the cursor instead just set the HIDE_CURSOR
flag to 1.

### Blur

You can add a background blur using a compositor. For instance
in *picom*:

    blur: {
      method = "dual_kawase";
      strength = 5;
      background = true;
      background-frame = true;
      background-fixed = true;
    }

    # Exclude conditions for background blur.
    # blur-background-exclude = []
    blur-background-exclude = [
      "window_type = 'dock'",
      "window_type = 'desktop'",
      "_GTK_FRAME_EXTENTS@:c",
      "class_g = 'Dunst'"
    ];

This will blur all backgrounds on translucent ARGB windows, expect
for those in the exclude rule. This might improve
readability and also appearance according to one's preference, but
will generally have a lesser impact on most windows since they tend
to render with high opacity values. *tlock* can be set to be fully
transparent, achieving a powerful background blur.

### Password

tlock will try to match your user login password by default.

You can choose to set a custom password by storing it in a specific
file.

    mkdir ~/.config/tlock
    cd ~/.config/tlock
    echo my_psw > passwd 
    chmod 600 passwd

You should also set the *CONFIG_DIR* and *PASSWD* constants
so they point to the preferred location. These shown here
are the default values, so you only need to uncomment
the flags (if they are commented) to use them, but otherwise
you can point them to any file.

Since I couldn't bear having to type the same password when
I only had one free hand rather than two and vice versa, you can
now add multiple newline separated passwords (one per line) like so:

    echo foo >> passwd 
    echo bar >> passwd 

### Alarm

You can play a sound alarm on any incorrect password
attempt. By default the program uses `play` to generate a synth
sound. *play* can be
found in the `sox` package, available on most distros community
repo, but you can set the custom
sound file flag to play any audio file on your computer
instead.

### Shutdown

You can set an automatic shutdown after a certain number of tries,
or if a CTRL+ALT+F1-13 or ALT+SYSRQ key sequence is 
recognized, trying to switch the tty or to kill X.

Number of tries and options such as disabling
CTRL+ALT+Backspace and other ways to kill the X server
during shutdown are all configurable in the config file.

Automatic shutdown requires sudo privileges to be set in
your sudoers file. Open the file for writing with:

    visudo 

Then add the following lines:

    # Options for poweroff
    [username] [hostname]= NOPASSWD: /usr/bin/systemctl poweroff
    [username] [hostname]= NOPASSWD: /usr/bin/shutdown -h now

    # SYSRQ options
    [username] [hostname] =NOPASSWD: /usr/bin/tee /proc/sys/kernel/sysrq

Where [username] and [hostname] are your username (or a
user group such as `%wheel`) and hostname (or use `ALL=`
to refer to any host) on the machine respectively.

You can combine this feature with a BIOS password as well as 
encrypted home and swap partitions. Meaning that once
your machine is shut off, your data is no longer
accessible in any way.

To ensure the OOM-killer is disabled, sudo can be used
internally. This requires another sudoers option:

    [username] [hostname]= NOPASSWD: /usr/bin/tee /proc/[0-9][0-9]*/oom_score_adj

**However, this is not recommended as now any process can 
modify the `oom_score` for any other process.**

### Preventing USB tampering

[GRSecurity support for Arch was dropped](https://lists.archlinux.org/pipermail/arch-general/2017-April/043604.html)
so we removed the feature and currently have no option to
deal with this.

### Webcam snapshot

You can enable the option to take a webcam shot of
whoever is tampering with your machine before shutdown.
A fun little trick legacy from chjj's work.

*Requires ffmpeg.*

## Requirements

In order to build tlock you need the Xlib header files.

Potential runtime deps (depending on options set and
configuration): sudo, ffmpeg, setxkbmap, sox.

## Install

First you need to clone this repo on your machine:

    git clone https://github.com/cherrynoize/tlock

You can then edit the `config.mk` file before you compile
the package to best suit your needs. By default it will be
installed as `/usr/local/bin/tlock`, but you can edit
the make config to install to any other location of your choosing. 

When you're satisfied with your configuration:

    make clean install

Anytime you edit the config file the above command must be rerun
before any new options become available.

> Please see [Additional configuration](#additional-configuration)
> below for more details.

## Usage 

    tlock

Then either enter your password or your tlock password,
if you have any. Et voilà.

### Auto-enable after suspend

> See [Troubleshooting](#troubleshooting) for issues with resume.

Create a new `resume@.service` file in
`/etc/systemd/system` or edit with care it if you
already have one:

    [Unit]
    Description=User resume from suspend actions
    After=suspend.target

    [Service]
    User=%I
    Environment=DISPLAY=:0
    ExecStart=/usr/local/bin/tlock

    [Install]
    WantedBy=suspend.target

Then enable and start the service for your user like this:

    systemctl enable --now resume@USER.service

Where USER is your username.

Verify it is working as expected with:

    systemctl status resume@USER.service

You can now suspend your system:

    systemctl suspend

And on resume tlock should be run. 

## Additional configuration

### DPMS settings

We chose to follow original slock on removing DPMS settings, so
you can easily customize them separately.

Here's an example script that you could run instead of tlock from
your systemd service:

    #!/bin/sh
    # Stop music or ignore if not playing
    /usr/bin/playerctl pause || true
    # Close all notifications
    /usr/bin/dunstctl close-all
    # Disable dpms
    /usr/bin/xset -dpms &
    # Run tlock
    /usr/local/bin/tlock
    # Re-enable dpms
    /usr/bin/xset +dpms

Of course this is all optional configuration but leaving DPMS
unhandled may constitute a security issue, for instance unlocking the
screen automatically after a certain timeout.

## Troubleshooting

### Screen flashing before locking

This is most likely not tlock's fault but the resume service's.

You can easily circumvent this by using the suspend service instead:

    [Unit]
    Description=User before suspend actions
    Before=sleep.target

    [Service]
    User=%I
    Environment=DISPLAY=:0
    ExecStart=/usr/local/bin/tlock

    [Install]
    WantedBy=sleep.target

Enable as for the resume service.

It may be briefly flashing the locked screen before suspending but
I believe that's better than the other way around.

---

## Contacts

> [u/cherrynoize](https://www.reddit.com/user/cherrynoize)
>
> [cherrynoize@duck.com](mailto:cherrynoize@duck.com)

Please feel free to contact me about any feedback or feature
request. Where possible, opt for a public issue instead.

## Donations

If you want to show your support (or just buy me a Pepsi 'cause
you're nice):

    ETH   0x5938C4DA9002F1b3a54fC63aa9E4FB4892DC5aA8

    SOL   G77bErQLYatQgMEXHYUqNCxFdUgKuBd8xsAuHjeqvavv

    BNB   0x0E0eAd7414cFF412f89BcD8a1a2043518fE58f82

    LUNC  terra1n5sm6twsc26kjyxz7f6t53c9pdaz7eu6zlsdcy

### Thank you for using tlock.
