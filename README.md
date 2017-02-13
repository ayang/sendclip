# sendclip
Send clipboard contents to other devices in lan. It can send text and screenshort from clipboard to other machines that installed sendclip too.

## compile
* To compile the project, qt5 is needed.
* Download [qhttp](https://github.com/azadkuh/qhttp) into parant directory of this project and setup.
* Compile it.

## download
* Download the compiled package from: http://pan.baidu.com/s/1jGJ2m6y

# usage
Sendclip is a system tray icon application. When you first run it, you need to make some settings. Your computers needs to use the same username, key, and port. Computers use the same username can send clipboard to each others. Key is used for encrypt and decrypt the messages using rc4 algrithom. Send to ip usually is a broadcast ip address, but it can be some ip addresses, one line per address.

Then you can click the icon to send clipboard contents to other machines. Good luck!
