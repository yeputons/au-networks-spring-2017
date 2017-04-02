#pragma once

#include <cstddef>
#include <exception>
#include <stdexcept>
#include <string>

/*
 * All the functions in the interface are blocking.
 */

struct stream_socket
{
    /*
     * Either sends all the data in buf or throws.
     * If exception is thrown then this socket would probably
     * throw them on all further sends.
     * Also in case of exception there is no guarantee about
     * the portion of data that has reached the other endpoint
     * and that has been processed by it.
     * You can't call this method in >1 threads simultaneously
     * - locking required;
     */
    virtual void send(const void *buf, size_t size) = 0;
    /*
     * Either reads size bytes of data into buf or throws.
     * Buf may be modified even if exception was thrown.
     * If exception is thrown then this socket would probably
     * throw them on all further recvs.
     * You can't call this method in >1 threads simultaneously
     * - locking required;
     */
    virtual void recv(void *buf, size_t size) = 0;
    virtual ~stream_socket() {};
};

struct stream_client_socket: stream_socket
{
    /*
     * If exception is thrown then this socket would probably
     * throw them on all further connects.
     */
    virtual void connect() = 0;
};

struct stream_server_socket
{
    /*
     * If exception is thrown then this socket would probably
     * throw them on all further accepts.
     */
    virtual stream_socket* accept_one_client() = 0;
    virtual ~stream_server_socket() {};
};

typedef const char* hostname;

class socket_uninitialized : public std::logic_error {
public:
  explicit socket_uninitialized(const std::string &what_arg) : logic_error(what_arg) {}
};

class socket_error : public std::runtime_error {
public:
  explicit socket_error(const std::string &what_arg) : runtime_error(what_arg) {}
};

class host_resolve_error : public socket_error {
public:
  explicit host_resolve_error(const std::string &what_arg) : socket_error(what_arg) {}
};

class socket_io_error : public socket_error {
public:
  explicit socket_io_error(const std::string &what_arg) : socket_error(what_arg) {}
};

class socket_eof_error : public socket_io_error {
public:
  explicit socket_eof_error(const std::string &what_arg) : socket_io_error(what_arg) {}
};
