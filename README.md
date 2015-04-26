# sendclip
Send clipboard contents to other devices in lan

I do my jobs on two computers, an ubuntu and a macbookpro. Many times when I need to send clipboard contents from one to another, I found there is no proper tool. So I made this. It may be can work on windows, but not tested yet.

## compile
To compile the project only need to install qt5.

## install
I have not made any binary to be downloaded, so, download the code and compile it.

# usage
Sendclip is a system tray icon application. When you first run it, you need to make some settings. Your computers needs to use the same username, key, and port. Computers use the same username can send clipboard to each others. Key is used for encrypt and decrypt the messages using rc4 algrithom. Send to ip usually is a broadcast ip address, but it can be some ip addresses, one line per address.

Then you can click the icon to send clipboard contents to other machines. Good luck!
