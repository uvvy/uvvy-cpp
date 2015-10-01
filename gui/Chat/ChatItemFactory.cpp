#include "ChatItem.h"
#include "ChatMessageItem.h"

ChatItem *ChatItem::createItem(ItemType itemType)
{
	switch(itemType)
	{
		case CHAT_MESSAGE_ITEM:
			return new ChatMessageItem();
		case CHAT_CALL_ITEM:
			return 0;
	}
}
