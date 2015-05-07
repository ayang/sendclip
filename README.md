# sendclip
Send clipboard contents to other devices in lan. It can send text and screenshort from clipboard to other machines that installed sendclip too. 

I do my jobs on two computers, an ubuntu and a macbookpro. Many times when I need to send clipboard contents from one to another, I found there is no proper tool. So I made this. It may be can work on windows, but not tested yet.

## compile
* To compile the project, qt5 is needed.
* Download [qhttpserver](https://github.com/nikhilm/qhttpserver) into parant directory of this project and make it (make install is unnecessary).
* Compile it.

## install
* MacOS and Windows: http://pan.baidu.com/s/1jGJ2m6y
* Linux: Download the code and compile it.

# usage
Sendclip is a system tray icon application. When you first run it, you need to make some settings. Your computers needs to use the same username, key, and port. Computers use the same username can send clipboard to each others. Key is used for encrypt and decrypt the messages using rc4 algrithom. Send to ip usually is a broadcast ip address, but it can be some ip addresses, one line per address.

Then you can click the icon to send clipboard contents to other machines. Good luck!
