# tlock | tightened simple screen locker

This is a fork of
[a fork of *slock*](https://github.com/chjj/slock)
from which we take inspiration for some sound security 
options but then actually take it back towards a more 
simplicistic approach (more like original *slock*'s) but 
with some added configuration capabilities.

## Screenshots

### Blur
![tlock screenshot](https://i.imgur.com/q69ovpS.png "tlock with kawase blur")
### Text
![tlock screenshot](https://i.imgur.com/goweCWJ.png "tlock with text message")
### Transparent
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
well as for the screen while typing, after a bad password
attempt and for the cursor color.

### Text

You can add a custom text message through the PRINT_MSG
value in `config.h`. FONT_COLOR is an #RRGGBB hex value
and FONT_NAME is an [XLFD font name](https://www.x.org/releases/X11R7.6/doc/xorg-docs/specs/XLFD/xlfd.html)
which sucks and you're never going to understand it fully. 

### Transparency

You can optionally enable transparency on your lock 
screen. In the config, just set TRANSPARENCY to 1.

> *Note: if the screen just goes black or if this is 
> otherwise not working as expected, you need to ensure
> that you are running a composite manager.*

To hide the cursor instead just set the HIDE_CURSOR
flag to 1.

### Password

You can choose to add a custom password by simply
creating a file and storing it in there.

    mkdir ~/.config/tlock
    cd ~/.config/tlock
    echo example_psw > passwd 
    chmod 600 passwd

Since I couldn't bear having to write the same password
when I had only one free hand and vice versa, you can
add multiple newline separated passwords (one for each
line) like so:

    echo example_psw_2 >> passwd 
    echo example_psw_3 >> passwd 

You also have to set the CONFIG_DIR and PASSWD macros
so they point to the correct file. These shown here
are the default values, so you only need to uncomment
the flags if they are commented, but otherwise you can
point them to any location of your choosing.

### Alarm

You can play a sound alarm on an incorrect password
attempt. By default the program uses `play`, which can be
found in the `sox` package, but you can set the custom
sound file flag to play any audio file on your computer
instead.

### Shutdown

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

### Preventing USB tampering

[GRSecurity support for Arch was dropped](https://lists.archlinux.org/pipermail/arch-general/2017-April/043604.html)
so we removed the feature and currently have no option to
deal with this.

### Webcam snapshot

You can enable the option to take a webcam shot of
whoever is tampering with your machine before shutdown.
A nice little goody inherited from chjj's fork. 

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
installed as `/usr/local/bin/tlock`, so you need to edit
the make configuration if you want to write the output
file to a different location.

When you're satisfied with your configuration:

    make clean install

## Usage 

    tlock

Then either enter your password or your tlock password,
if you have any. Et voilà.

### Auto-enable after suspend

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

Verify it is working as expected with:

    systemctl status resume\@USER.service

---

## Contacts

> u/cherrynoize
> 
> 0xo1m0x5w@mozmail.com

Please feel free to contact me about any feedback or
request.

## Donations

If you wanted to show your support or just buy me a Coke:

    ETH   0x5938C4DA9002F1b3a54fC63aa9E4FB4892DC5aA8

    SOL   G77bErQLYatQgMEXHYUqNCxFdUgKuBd8xsAuHjeqvavv

    BNB   0x0E0eAd7414cFF412f89BcD8a1a2043518fE58f82

    LUNC  terra1n5sm6twsc26kjyxz7f6t53c9pdaz7eu6zlsdcy

### Thank you for using tlock.
