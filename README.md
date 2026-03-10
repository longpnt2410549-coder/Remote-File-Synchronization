**B4 Remote File Synchronization**



summary:

This tool synchronizes two directories—one on a Client and one on a Server—over a network using TCP sockets. It implements a "Delta Sync" approach, meaning it only transfers files that have been modified or are missing, saving bandwidth and time.



---The tool follows a Request-Response model over a persistent TCP connection.---

1. The client and server both scan their respective directories using <dirent.h> and <sys/stat.h>.



2\. The client sends its file list (Name, Size, Mtime) to the server.



3\. The server compares the client's list with its own and generates a "Sync Plan" (Uploads, Downloads, Deletions, or Skips).



Execution: The client receives the plan and executes the transfers using a custom Message Framing protocol.



---TCP setup---

Step 1: Send the MsgHeader (fixed size).



Step 2: Send the Payload (variable size, e.g., raw file bytes).



Step 3: Receiver reads the header, then loops recv() until payload\_size is reached.





---functions---







