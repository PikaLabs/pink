pinkrpc is a simple remote procedure caller framework, which is based on pink, a network-programming library. Following is a quick start:

1. Get The pink repository and check to pbrpc branch

   ```
   git clone -b pbrpc https://github.com/Qihoo360/pink
   ```

2. Get code generator bin file

   ```
   make
   make rpc
   ```

   the first "make" is to get the protobuf and the pink library; the second "make rpc" is to get "code_gen" bin file. After that you will find the code_gen file in the $(PINK_ROOT)/output/bin/ directory 

   **PS:** Here, we use protobuf 3 and the protobuf library is obtained by compiling the source code, which is got by using wget command in the first "make" period. If the network is too poor for the wget command, you can in advance download the protobuf source code in other ways and put it in the $(PINK_ROOT)/deps/ directory with the name "protobuf"(which should be a unpacked directory).

3. define your service

   arbitrarily enter a directory, open a new photo file(here is sample.proto),  and define the service apis you want, for example:

   ```
   //sample.proto
   syntax = "proto3";
   package pink.sample;
   option cc_generic_services = true;

   message Request {
     string req = 1;
   }
   message Response {
     string res = 1;
   }
   service EchoService {
     rpc echo(Request) returns(Response);
   }
   ```

4. generate the rpc code

   In the sample.proto file's directory, execute the following command

   ```
   $(PINK_ROOT)/output/bin/code_gen sample.proto
   ```

   you will see the generated code files. Among them, 

   **sample_impl.cc is the file where you want to implement your own apis,**

   **sample_client_main.cc is a sample client main source file,** 

   **and the above two files shoud be noticed**

   PS: 

   1. proto file name is ok, don't add the path. 

   ```
     $(PINK_ROOT)/output/bin/code_gen sample.proto             //OK
     $(PINK_ROOT)/output/bin/code_gen ./sample.proto           //wrong
   ```

   2. Don't change the pink's source_code path.

       You can copy/move "code_gen" to any directory, and it is valid. But once the pink source code's path has been changed, the code_gen, previously compiled, is invalid and needs recompiled.

5. compile the generated code

   In the sample.proto file's directory, execute

   ```
   make
   ```

   and you will get a new directory named "output", which contains:

   sample_server_main                        //server bin file

   sample.conf                                       //server conf file

   sample_client_main                         //client sample bin file

   sample_client.h                                //libsample.a's header file

   libsample_client.a                            //client library

6. Run

   1. run server

      ```
      ./sample_server_main -c sample.conf
      ```

   2. run client

      ```
      ./client_client_main
      ```

     

The client sample is very simple, for usage reference, and you can implement your own more smart client

**Enjoy it!!!**

​       
