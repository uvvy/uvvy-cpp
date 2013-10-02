Messaging system
==============

Messaging needs to be a mix between real-time chat and non real-time email.

Each conversation starts very simple and has basic controls. Conversation can be enhanced by additional features and invite more contacts afterwards. This keeps start interface simple and suitable for quick one-time messaging, but allows for further expansion if there is a need.

Conversation starts simply by picking a peer and sending that peer a message.

Messages are like git commits. Messages are stored authenticated and also optionally encrypted. They include timestamp, author, conversation ID, optionally subject and other additional attributes and most importantly, parent message(s) IDs.

That's right, a message can have several parents. By default the chat flow is linear and messages have only one parent, but nothing prevents you from selecting multiple messages to answer to at once.

Protocol allows you to easily detect gaps in the conversation. When you receive a message and it's parent is not in the local database, you can query your peer for missing messages and receive them quickly in one roundtrip.

One negative consequence of adopting git-like model for messages is editing. When you change a previous message, its hash changes, invalidating parent pointers in all following messages. When you modify the next message to point back to the modified parent, its hash also changes, resulting in a cascade of changes to all the following messages, which need then be resynchronised to other peers. This can be problematic if the messages are signed by their authors.

@todo Investigate how bad it could be.
