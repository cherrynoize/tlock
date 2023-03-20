# tlock | tightened simple screen locker

This is a fork of [a fork of *slock*]
(https://github.com/chjj/slock) from which we take
inspiration for some sound security options but actually
take it back towards a more simplicistic approach like
the original *slock*'s but with some added configuration
capability.

## Configuration

All configuration is done in the `config.h` file.

### Colors

Colors are fully customizable from config file.

You can choose colors for the initial lock screen, as
well as the screen when typing, after a bad password
attempt and for the cursor color.

### Transparency

You can optionally enable transparency on your lock 
screen. In the config, just set TRANSPARENCY to 1.

> *Note: if the screen just goes black or if this is 
> otherwise not working as expected, you need to ensure
> that you are running a composite manager.*

To hide the cursor instead just set the HIDE_CURSOR
flag to 1.

## Password

You can choose to utilize a custom password by simply
creating a file and storing it in there.

    mkdir ~/.config/tlock
    cd ~/.config/tlock
    echo example_psw > passwd 
    chmod 600 passwd

You also have to set the CONFIG_DIR and PASSWD macros
so they point to the correct file. These are the default
values, so you only need to uncomment them if they are
commented, but otherwise you can point them to any
location of your choosing.

> *Warning: currently experimental. May not find your
> password file and default to your login password as
> normal.*

## Alarm

You can play a sound alarm on each incorrect password
attempt. By default the program uses `play`, which can be
found in the `sox` package, but you can set the custom
sound file flag to play any audio file on your computer
instead.

## Shutdown

You can set an automatic shutdown after too many tries,
or if a CTRL+ALT+F1-13 or ALT+SYSRQ key sequence is 
recognized, trying to switch tty or kill X.

Number of tries and options such as disabling
CTRL+ALT+Backspace and other ways to kill the X server
during shutdown are all configurable in the config file.

Automatic shutdown requires sudo privileges to be set in
your sudoers file.

    visudo 

Then add the following lines:

    # Options for poweroff.
    [username] [hostname]= NOPASSWD: /usr/bin/systemctl poweroff
    [username] [hostname]= NOPASSWD: /usr/bin/shutdown -h now

    # SYSRQ options.
    [username] [hostname] =NOPASSWD: /usr/bin/tee /proc/sys/kernel/sysrq

Where [username] and [hostname] are your username (or a
user group such as `%wheel`) and hostname (or use `ALL=`
to refer to any host) on the machine respectively.

As [chjj](https://github.com/chjj/slock) points out you
can combine this feature with a BIOS password as well as 
encrypted home and swap partitions. Meaning that once
your machine is shut off, your data is no longer
accessible in any way.

To ensure the OOM-killer is disabled, sudo can be used
internally. This requires another sudoers option:

    [username] [hostname]= NOPASSWD: /usr/bin/tee /proc/[0-9][0-9]*/oom_score_adj

However, this is not recommended as now any process can 
modify the `oom_score` for any other process.

## Preventing USB tampering

[GRSecurity support was dropped](https://lists.archlinux.org/pipermail/arch-general/2017-April/043604.html)
so we removed the feature and currently have no option to
deal with this.

## Webcam snapshot

You can enable the option to take a webcam shot of
whoever is tampering with your machine before shutdown.

*Requires ffmpeg.*

## Requirements

In order to build tlock you need the Xlib header files.

Potential runtime deps (depending on options set and
configuration): sudo, ffmpeg, setxkbmap, sox.

## Installation

You can edit the `config.mk` file before you compile the
package to suit it to your needs. By default it will be
installed as `/usr/local/bin/tlock`, so you need to edit
the make configuration if you want to write the output
file to a different location.

When you're satisfied with your configuration:

    make clean install

## Usage 

    tlock

Then either enter your password or your tlock password,
if you have any. Et voilà.

## Auto-enable after suspend

Create a new `resume@.service` file in
`/etc/systemd/system` or edit with care it if you
already have one:

    [Unit]
    Description=User resume from suspend actions
    After=suspend.target

    [Service]
    User=%I
    Type=forking
    Environment=DISPLAY=:0
    ExecStart=/usr/local/bin/tlock

    [Install]
    WantedBy=suspend.target

Then enable or start the service for your user like this:

    systemctl enable resume\@USER.service
    # or
    systemctl start resume\@USER.service

Where USER is your username.

Verify it is working properly with:

    systemctl status resume\@USER.service

#### Thank you for using tlock.
