# Simple-Dropbox
File synchronization through network.

## dropbox_server  
Compilation: make  
Execution: ./dropbox_server -p portNum

## dropbox_client  
Compilation: make  
Execution: ./dropbox_client –i inputDir -m mirrorDir –p portNum –w workerThreads –b bufferSize –sp serverPort –shm serverhostname
Where:  
- inputDir: is the directory that contains the files that the client wants to share.
- inputDir: is the directory where this client will store the files of the rest dropbox_clients that participate in the file exchange
- portNum: is the port where dropbox_client is listening in order to receive messages from the dropbox_server or other dropbox_clients
- workerThreads: the number of WorkerThreads that the dropbox_client will create in order to complete the requests that are stored in the buffer
- bufferSize: the size of the cyclic buffer which is shared between the dropbox_client's threads
- serverPort: the port number of the dropbox_server to which this client will connect
- serverhostname: the port number of the dropbox_server to which this client will connect

Example:  
./dropbox_client –i Client1/InputDir/ -m Client1/MirrorDir/ –p 8080 -w 2 –b 5 –sp 5050 –shm linux09.di.uoa.gr
./dropbox_client –i Client2/InputDir/ -m Client2/MirrorDir/ –p 8081 -w 2 –b 5 –sp 5050 –shm linux09.di.uoa.gr

