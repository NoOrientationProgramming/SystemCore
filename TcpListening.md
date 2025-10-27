
# TcpListening() Manual Page

## ABSTRACT

Class for listening and accepting incoming TCP connections.
Created sockets are stored in a queue.

## LIBRARY

SystemCore

## SYNOPSIS

```cpp
#include "TcpListening.h"

// creation
static TcpListening *create();

// configuration
void portSet(uint16_t port, bool localOnly = false);
void maxConnQueuedSet(size_t maxConn);

// result
SOCKET nextPeerFd();
Pipe<SOCKET> ppPeerFd;
```

## DESCRIPTION

The **TcpListening()** class allows setting up a listening socket to accept incoming **TCP connections** on IPv4 and IPv6.
For each new connection, the file descriptor (socket handle) is provided to the user via the **ppPeerFd** pipe or the `nextPeerFd()` getter.

Internally, **TcpListening()** uses standard POSIX socket functions (`socket()`, `bind()`, `listen()`, `accept()`)
and the equivalent Winsock API on Windows platforms.
Both IPv4 and IPv6 listeners can be created simultaneously if supported by the system.

## CREATION

### `static TcpListening *create()`

Creates a new instance of the **TcpListening()** class.
Memory is allocated using `new` with the `std::nothrow` modifier to ensure safe handling of failed allocations.

## CONFIGURATION

Before starting the listening process, one or more parameters must be configured.
After that, the **TcpListening()** process can be started asynchronously using the **Processing()** mechanism.

### `void portSet(uint16_t port, bool localOnly = false)`

Sets the TCP port number to listen on.

* **port**: TCP port (e.g., 8080).
* **localOnly**: When true, the socket will only bind to the loopback interface (`127.0.0.1` or `::1`).
  When false (default), the socket will be accessible from all interfaces.

### `void maxConnQueuedSet(size_t maxConn)`

Sets the maximum number of pending connections allowed in the TCP backlog.

* **maxConn**: Maximum number of pending connections (e.g., 16).

## START

### `Processing *start(Processing *pChild, DriverMode driver = DrivenByParent)`

Once the process is started, it progresses "in the background".
This means that with each system tick, the process is allowed to take a small amount of processing time.
During each tick, the process must account for other processes that are contained within the same driver tree.

The progression can be managed by the parent process itself (DrivenByParent = default) or optionally by a new driver.
When a new driver is used, it creates a new driver tree.
All children within a driver tree share the processing time of the system ticks, unless a new driver tree is created.

A new driver can either be an internal driver, such as a worker thread (DrivenByNewInternalDriver),
or any external driver (DrivenByExternalDriver), like a thread pool or a specialized scheduler.

- **pChild**: Pointer to any process which is derived from **Processing()**.
- **driver**: Type of driver which is responsible for the progress of the new child process. A new thread? --> DrivenByNewInternalDriver

## SUCCESS

### `Success success()`

Processes are related to functions.
They establish a **mapping from input to output**.
For **functions**, the **mathematical signature** is **y = f(x)**.
In the case of processes, however, the mapping cannot happen immediately as with functions;
instead, it takes too much time to wait for completion.
Therefore, the mathematical signature of **processes** is **y = p(x, t)**.

In **software**, processes also differ from functions.
While **functions** are managed by the compiler and the calling procedure (ABI) on the system's **stack**,
**processes** must be managed by the user and reside in the **heap** memory.

As long as this process is not finished, its function **success()** returns **Pending**.
On error, success() is **not Positive** but returns some negative number.
On success, success() returns **Positive**.

## RESULT

Each time a new client connects, a new socket descriptor (peer file descriptor) becomes available.
It can be retrieved either through the `nextPeerFd()` getter or from the **ppPeerFd** pipe.

### `SOCKET nextPeerFd()`

Returns the next accepted peer socket descriptor.
If no connection is pending, the function returns **INVALID_SOCKET**.

### `Pipe<SOCKET> ppPeerFd`

This `Pipe` is essentially a **thread-safe queue** for `SOCKET` values.

Elements can be retrieved using the get() function:
```cpp
ssize_t get(PipeEntry<T> &entry);
```

The function returns:

* 1 – if a particle was successfully fetched (the entry is valid),
* 0 – if no particle is currently available,
* -1 – if no more particles are available (the pipe has been closed or drained).

A PipeEntry contains two key components:

* `particle` — the element of the template type (here: a `SOCKET`)
* two values `t1` and `t2` of type `ParticleTime` — timestamp/metadata objects associated with the particle

Notes:

* The pipe is thread-safe, so the listener and consumers (other processes) may safely push and pull entries concurrently
* Each entry carries both the socket and its `ParticleTime`, which lets consumers know when the socket was accepted

## EXAMPLES

### Example: Listening for Incoming Connections

In the header file of **ChatServing()**:

```cpp
  /* member variables */
  TcpListening *mpLst;
```

In the source file of **ChatServing()**:

Using `nextPeerFd()`.
```cpp
Success ChatServing::process()
{
  Success success;
  SOCKET fdPeer;

  switch (mState)
  {
  case StStart:

    mpLst = TcpListening::create();
    if (!mpLst)
      return procErrLog(-1, "could not create listener");

    mpLst->portSet(8080);

    start(mpLst);

    mState = StWaitConn;

    break;
  case StWaitConn:

    fdPeer = mpLst->nextPeerFd();
    if (fdPeer == INVALID_SOCKET)
      break;

    procInfLog("New peer connected on port 8080");

    ...

    break;
  default:
    break;
  }

  return Pending;
}
```

Using **ppPeerFd**.
```cpp
Success ChatServing::process()
{
  Success success;
  ssize_t numParticles;
  PipeEntry<SOCKET> peerFdEntry;
  SOCKET fdPeer;

  switch (mState)
  {
  case StStart:

    ...

    break;
  case StWaitConn:

    numParticles = mpLst->ppPeerFd.get(peerFdEntry);
    if (numParticles < 0)
      return procErrLog(-1, "listener pipe drained");

    if (!numParticles)
      break;

    fdPeer = peerFdEntry.particle;

    procInfLog("New peer connected on port 8080");

    ...

    break;
  default:
    break;
  }

  return Pending;
}
```

## SCOPE

* Linux
* FreeBSD
* macOS
* Windows (via Winsock2)

## RECURSION

```
Order                 1
Depth                 1
```

## NOTES

**TcpListening()** typically runs as a **service process**, whose lifespan matches that of its parent process.

## DEPENDENCIES

### SystemCore

The base structure for all software systems.

```
License               MIT
Required              Yes
Project Page          https://github.com/NoOrientationProgramming
Documentation         https://github.com/NoOrientationProgramming/SystemCore
Sources               https://github.com/NoOrientationProgramming/SystemCore
```

## SEE ALSO

**TcpTransfering()**, **accept(2)**, **socket(2)**, **bind(2)**, **listen(2)**

## COPYRIGHT

Copyright (C) 2024, Johannes Natter

## LICENSE

This software is distributed under the terms of the MIT License. See <https://opensource.org/licenses/MIT> for more information.

