# ft\_irc

> **42 Network – IRC Server Implementation in C++**

---

## 📚 Project Description

`ft_irc` is a 42 system programming project that requires building an IRC (Internet Relay Chat) server from scratch in C++. The goal is to support multiple clients using non-blocking I/O with `poll()`, handle user authentication, and implement basic IRC protocol commands such as NICK, USER, PASS, PRIVMSG, KICK, INVITE, and MODE.

---

## 🧠 Objectives

* Create an IRC server that can handle multiple simultaneous users.
* Support and correctly parse key IRC commands.
* Maintain user state (nickname, username, real name, etc.).
* Manage channels with permissions and user modes.
* Ensure compliance with basic IRC RFC 1459 behavior.

---

## ⚙️ Commands Implemented

| Command   | Description                                                  |
| --------- | ------------------------------------------------------------ |
| `NICK`    | Set or change a user's nickname.                             |
| `USER`    | Define a user's username and real name.                      |
| `PASS`    | Authenticate the user with a server password.                |
| `PRIVMSG` | Send a private message to a user or channel.                 |
| `KICK`    | Remove a user from a channel (requires operator privileges). |
| `INVITE`  | Invite a user to a channel.                                  |
| `MODE`    | Change channel modes using flags.                            |

---

## 🧾 Supported MODE Flags

| Mode | Description                                       |
| ---- | ------------------------------------------------- |
| `+i` | Invite-only channel. Only invited users can join. |
| `+k` | Channel requires a password (key) to join.        |
| `+o` | Grant/revoke operator privileges.                 |
| `+t` | Only channel operators can change the topic.      |
| `+l` | Limit the number of users in the channel.         |

---

## 🖥️ How to Use

1. **Compile the server:**

```bash
make
```

2. **Run the server:**

```bash
./ircserv <port> <password>
```

3. **Connect using an IRC client:**

```irc
/connect 127.0.0.1 <port>
```

4. **Register as a user:**

```
PASS <password>
NICK <nickname>
USER <username> 0 * :<realname>
```

---

## 🧪 Example Usage

```irc
JOIN #school42
PRIVMSG #school42 :Hello everyone!
MODE #school42 +i
INVITE john #school42
KICK #school42 spammer :Spamming
```

---

## 🧱 Technical Stack

* C++98 (required)
* Sockets (AF\_INET)
* poll() for handling multiple clients
* No external IRC libraries allowed

---

## ✅ Evaluation Criteria

* [x] Non-blocking server using `poll()`
* [x] Handles multiple clients concurrently
* [x] Implements core IRC commands
* [x] Correct nickname, user, and channel management
* [x] Clean C++ code, modular and maintainable
* [x] Handles malformed input and disconnections gracefully
