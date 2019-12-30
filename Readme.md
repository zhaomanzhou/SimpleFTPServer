a small remote backup system with a simple protocol to exchange information between server and
client. With this backup system, you can use the client to list whatever files that have been stored in the
remote server’s backup folder. You can also send a local file to be stored in that backup folder on the server.
If you do not want a file any more, you have the client send command to remove that files from the server’s
backup folder.

This protocol includes five command functions

- ls
This function lists the available files in the server’s backup folder
- send
This function sends a specific file to the server and the server will store it in its backup folder
command syntax  `send <file path>`
- remove
This function removes a file from the server’s backup folder
command syntax  `remove <file path>`
- quit
quit the client
-shutdown
shutdown the server

#build it

`cmake CmakeLists.txt`
`make`

# run it
- server
    `./server <-p port>`
- client
    `./client < -s address> [-p port]`