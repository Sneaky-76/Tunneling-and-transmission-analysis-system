# Tunneling and transmission analysis system
Repository for a project combining subjects Network Programming with Telecommunication Networks and Systems, conducted in the year 2025/26.
The goal of the project is to make a functional tunneling system, consisting of both client and server applications, with the analysis of its
transmission parameters. It also include user databse with logging and registration, written in C++ language. No .exe files involved :)

Project team:

Paweł Ścibek

Kacper Olszewski

Wiktor Rogowski

##  Prerequisites

This project is designed for **Linux** operating systems. Before compiling, ensure you have the following packages installed:

* **g++ Compiler**
* **make utility**
* **libsodium library** (cryptography)
* **libsctp library** (SCTP protocol support)

### Installation (Ubuntu/Debian)

```bash
sudo apt-get update
sudo apt-get install build-essential libsodium-dev libsctp-dev
```
### Compilation
Software needs to be run from more then one Linux machines connected in the same network. If machines are virtual then for example in Oracle VirtualBox nat network works.
For server: in `project_tuna/tuna_srv/core`
```bash
make
```

For client: in `project_tuna/tuna_cli/core`
```bash
make
```

### Usage
For server in `project_tuna/tuna_srv/core`:
```bash
sudo ./serverSide <protocol>
```
tcp, udp or sctp is supported 

For client in `project_tuna/tuna_cli/core`:
```bash
sudo ./clientSide <server_ip4> <protocol>
```
tcp, udp or sctp is supported

1. Authentication: Upon start, a menu is displayed. Choose option 2 to Register (if new) or 1 to Login.
2. Background Traffic: After successful login, a prompt to activate the background traffic generator is shown (confirm with <y/n>)
   alongside with packet loss simulation tool prompt accepting values from 0 to 100 [%].
3. Messaging: Enter messages to send them to the server. Type 'q' to quit the application.

###Notes:
- The server initializes a local database file ('users_db.txt') for user authentication.
- Administrator privileges (sudo) are required for binding to system ports.
- Supported protocols usually include: tcp, udp, sctp.
- more then 1 clients are supported too





