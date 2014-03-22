Messaging system
================

Messaging needs to be a mix between real-time chat and non real-time email.

Each conversation starts very simple and has basic controls. Conversation can be enhanced by additional features and invite more contacts afterwards. This keeps start interface simple and suitable for quick one-time messaging, but allows for further expansion if there is a need.

Conversation starts simply by picking a peer and sending that peer a message.

Messages are like git commits. Messages are stored authenticated and also optionally encrypted.

They include
+timestamp,
+author, (originator_name/originator_eid)
+conversation ID,
optionally subject ?
and other additional attributes and most importantly,
parent message(s) IDs.

     Message has a timestamp, originator and the target chat. Synchronize based on that. Chat should pull
     non-existant entries once activated. How to track joins/leaves (+ history available to new joiners)?
     (unread, originator_name, timestamp, target, msg_hash) - tuple for syncing messages.
     Unread is synced, for example if the chat was already read on a device, then unread=0 may be synced.
     (originator_name, originator_eid, originator_timestamp) - to track source of the message.

That's right, a message can have several parents. By default the chat flow is linear and messages have only one parent, but nothing prevents you from selecting multiple messages to answer to at once.

Protocol allows you to easily detect gaps in the conversation. When you receive a message and it's parent is not in the local database, you can query your peer for missing messages and receive them quickly in one roundtrip.

When you change a previous message, it creates kind of a branch, rooted in the original message. Edit is a full new message, which has the original message as its parent. Messages within the branch are sorted by timestamp, the latest one represents the last actual contents of the edited message. (There needs to be a way to distinguish branch head from the main conversation head).
Instead of specifying a parent, we specify it as an EditOf: field, giving previous UUID there.
Both messages should have the same parent field.

Since messages are immutable, it's easy to refer to previous messages in the conversation. Quoting system relies on this by pointing to subranges within previous message IDs.

Conversation has by default a history horizon - point until which all participants can see the conversation history. It is simply a message ID. By default, when starting a group conversation, this is set to immediately after the forking point so that newly added peers do not see the private conversation before.
This point can be arbitrarily changed by the conversation initiators.

How to track joins/leaves (+ history available to new joiners)?

Using special message types for that. When a user joins, a parent message is recorded to indicate
from which point use should start receiving history, if history is not globally exposed. Otherwise
it marks a point from which older history must be synced.

Email introduces certain properties into messaging:
- the storage should be responsibility of the sender until you accept the message, this prevents mass mailing;
- sender must be authenticated, but it may be possible to receive email from unknown senders;
- emails usually take longer to compose, it is not a realtime media;
- emails may carry attachments.

First two points are automatically provided by the protocol. To communicate two nodes mutually authenticate.

The third point is entirely in UI space and can be solved by providing additional composing mode, where text editing is more like a regular Word document and sending is a separate controlled action.

The last point is rather easy to have, as in mettanode protocol any message including realtime chat messages may carry additional payload. Either inline, or as a reference to separately shared file blob.

### Offline messaging

A node going offline voluntarily might need to flush all undelivered messages to several of its peers for delivery.
Messages need to be encrypted with recipient public key.

A node that may go offline involuntarily shall flush undelivered messages to some of its peers periodically.

### One to one chat room

```plain
                      +---------------------------------+<-- history horizon (acecabaceca)
                      |By: Alice                        |              |
  +-----------+       |To: chat[alice<->bob] <- UUID?   |           +----------+
  |           |       |Parent: none (root message)      |           |          |
  |   Alice   |------>|Date: 2013.11.11 19:12 UTC       |---------->|   Bob    |
  |           |       |Text: Hi bob, how are you?       |           |          |
  +-----------+       |UUID: acecabaceca                |           +----------+
         ^            |HMAC: sha512256                  |<-+                 |
         |            +---------------------------------+  |                 |
         |              ^                                  |                 |
         |            +---------------------------------+  |                 |
         |            |By: Bob                          |  |                 |
         |            |To: chat[alice<->bob]            |  |                 |
         |            |Parent: acecabaceca              |  |                 |
         |            |Date: 2013.11.11 19:17 UTC       |  |                 |
         |            |Text: Hi alice, I'm fine thanks  |  |                 |
         |            |UUID: becacecafe                 |  |                 |
         |            |HMAC: sha512256                  |  |                 |
         |            +---------------------------------+  |                 |
         |               ^             +---------------------------------+   |
         |               +-------------|Edit of: becacecafe              |   |
         |                             |By: Bob                          |<--+
         +-----------------------------|To: chat[alice<->bob]            |
                                       |Parent: acecabaceca              |
                                       |Date: 2013.11.11 19:18 UTC       |
                                       |Text: Hi alice, I'm good thanks  |
                                       |UUID: feeb1ef1ea                 |
                                       |HMAC: sha512256                  |
                                       +---------------------------------+
```

parent (parent_message_id)
edit_of (edited_message_id)
timestamp (source_timestamp, target_timestamp) - to measure interdevice synchronization time.
type (message_type - text/picture/join/leave/topic/action/filesend/etc)
text (message text, may be dependent on message type?)
metadata (additional metadata blob describing special types of messages, e.g. joins/leaves etc)
hmac  (hmac over several message fields, see list below)
unread (local status if this message has been read by the user or not)


How to hmac, encrypt and sync messages in 3-way or more chat (with more than 2 participants)?

Chat ID must change when 3rd participant joins.
Probably by adding certain random data at the end of the original chat id.

How to extend history horizon to messages in the old 1-1 chat?

Probably not use source-target eids for chat id. Make a completely random generated string of data?
This needs an extra database check to see if we already have chat history with certain eid.


## Chat protocol

On the wire message format.

Messages are sent on the chat stream as separate records.
Records are encoded in flurry format.

Message components

source_name Originator display name, recorded at the moment message was sent.
            On the receiving side could be replaced with locally set display name for the contact.
            Not HMACed.

source_eid Originator's EID, set by original sender.

target_chat_id Unique ID of the chat this message belongs to. When new chat is created, a new unique
               ID is generated. Every client stores chat ID associated with each peer it ever
               chatted with.
               Chat ID may change when spawning a group chat (duh?)

