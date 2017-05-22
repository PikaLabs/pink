## Pink

[![Build Status](https://travis-ci.org/Qihoo360/pink.svg?branch=master)](https://travis-ci.org/Qihoo360/pink)

Pink is a wrapper of pthread. Why you need it?

When you use pthread, you always define many type of thread, such as:

In network programming scenario, some thread used for accept the client's connection, some thread used for pass
message to other thread, some thread used for communicate with client using protobuf.

So pink wrap a thin capsulation upon pthread to offer more convinent function.

### Model

DispatchThread + Multi WorkerThread

![](http://i.imgur.com/XXfibpV.png)

Now pink support these type of thread:
#### DispatchThread: 

DispatchThread is used for listen a port, and get an accept connection. And then dispatch this connection to the worker thread. the usage example is:

```
Thread *t = new DispatchThread<BadaConn>(9211, 1, reinterpret_cast<WorkerThread<BadaConn> **>(badaThread));

```

You can see example/bada.cc for more detail example

#### WorkerThread:

WorkerThread is the working thread, working thread use to communicate with
client, you can use different protocol, now we have support google protobuf
protocol. And you just need write the protocol, and then generate the code. The
WorkerThread will deal with the protocol analysis, so it simplify your job.

the usage example is:

```
class BadaThread : public WorkerThread<BadaConn>
```

You can see example/bada_thread.h example/bada_thread.cc for more detail example

#### HolyThread:

HolyThread just like the redis's main thread, it is the thread that both listen a port and do
the job. When should you use HolyThread and When should you use DispatchThread
combine with WorkerThread, if your job is not so busy, so you can use HolyThread
do the all job. if your job is deal lots of client's request, we suggest you use
WorkerThreads

the usage example is:

```

class PinkThread : public HolyThread<PinkHolyConn>

Thread *t = new PinkThread(9211);
t->Start();

```

You can see example/holy_test.h example/holy_test.cc for more detail example

Now we will use pink build our project pika, floyd, zeppelin

In the future, I will add some thread manager in pink.
