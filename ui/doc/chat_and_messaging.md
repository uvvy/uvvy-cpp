Messaging system
==============

Messaging needs to be a mix between real-time chat and non real-time email.

Each conversation starts very simple and has basic controls. Conversation can be enhanced by additional features and invite more contacts afterwards. This keeps start interface simple and suitable for quick one-time messaging, but allows for further expansion if there is a need.

Conversation starts simply by picking a peer and sending that peer a message.

Messages are like git commits. Messages are stored authenticated and also optionally encrypted. They include timestamp, author, conversation ID, optionally subject and other additional attributes and most importantly, parent message(s) IDs.

That's right, a message can have several parents. By default the chat flow is linear and messages have only one parent, but nothing prevents you from selecting multiple messages to answer to at once.

Protocol allows you to easily detect gaps in the conversation. When you receive a message and it's parent is not in the local database, you can query your peer for missing messages and receive them quickly in one roundtrip.

When you change a previous message, it creates kind of a branch, rooted in the original message. Edit is a full new message, which has the original message as its parent. Messages within the branch are sorted by timestamp, the latest one represents the last actual contents of the edited message. (There needs to be a way to distinguish branch head from the main conversation head).

Since messages are immutable, it's easy to refer to previous messages in the conversation. Quoting system relies on this by pointing to subranges within previous message IDs.

Conversation has by default a history horizon - point until which all participants can see the conversation history. It is simply a message ID. By default, when starting a group conversation, this is set to immediately after the forking point so that newly added peers do not see the private conversation before.
This point can be arbitrarily changed by the conversation initiators.

Email introduces certain properties into messaging:
- the storage should be responsibility of the sender until you accept the message, this prevents mass mailing;
- sender must be authenticated, but it may be possible to receive email from unknown senders;
- emails usually take longer to compose, it is not a realtime media;
- emails may carry attachments.

First two points are automatically provided by the protocol. To communicate two nodes mutually authenticate.

The last point is rather easy to have, as in mettanode protocol any message including realtime chat messages may carry additional payload. Either inline, or as a reference to separately shared file blob.

The third point is entirely in UI space and can be solved by providing additional composing mode, where text editing is more like a regular Word document and sending is a separate controlled action.
