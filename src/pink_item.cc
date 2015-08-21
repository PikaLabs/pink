#include "pink_item.h"
#include "pink_define.h"
#include <sys/stat.h>
#include <sys/socket.h>
#include <fcntl.h>
#include <unistd.h>
#include "csapp.h"

PinkItem::PinkItem(int fd) :
    fd_(fd)
{
}

PinkItem::~PinkItem()
{
}
