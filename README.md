# message-server
Publish/Subscribe Message Server created for 2020/1 Computer Network course at UFMG

Run make and two binaries cliente and servidor should be created

Run the server using `./servidor <port>` Run the server using `./cliente 127.0.0.1 <port>`

Clientes can subscribe to tags using `+` and unsubscribe using `-`

e.g.: `+github` `-github`

Tags must be composed only by letters, numbers and symbols are not allowed in tags and subscription/unsubscription messages only support one tag at a time
