Readme for the socket version
-----------------------------

The new version needs googles protobuf and the correspondig c-bindings
(protobuf-c). If you intend to modify the protocol you'll need the profbuf
compiler as well. These are separate packages on some distributions (e.g. 
Ubuntu). 
Once the project builds you can start working on it. The debugger is unable
to launch the debuggee locally right now. It's easy to implement it but 
right now a few bits are missing. 
To launch the debuggee you can use the launch scripts in bin/.
Launch-glxgears.sh starts glxgears and stops execution in the first GL-call. 
Run these scripts from the root folder of the project or adopt them as needed!
To interact with the debuggee you can use the cli client (bin/cli). Most of 
the commands are not implemented! Basically, you can just connect ("c") to the  
debuggee and request the current GL call ("f" key). Stepping etc. is not 
yet. It's almost done though.
