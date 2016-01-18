#include "pink_item.h"
#include "pink_define.h"
#include <sys/stat.h>
#include <sys/socket.h>
#include <fcntl.h>
#include <unistd.h>
#include "csapp.h"

namespace pink {

PinkItem::PinkItem(const int fd, const std::string &ip_port) :
    fd_(fd),
    ip_port_(ip_port)
{
}

PinkItem::~PinkItem()
{
}

}
