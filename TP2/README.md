# message-server

Publish/Subscribe Message Server created for 2020/1 Computer Network course at UFMG

Run make and two binaries client and server should be created

Run the server using `./server <port>` Run the server using `./client 127.0.0.1 <port>`

Clientes can subscribe to tags using `+` and unsubscribe using `-`

e.g.: `+github` `-github`

Tags must be composed only by letters, numbers and symbols are not allowed in tags and subscription/unsubscription messages only support one tag at a time
