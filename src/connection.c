/*
 * Copyright © 2008 Kristian Høgsberg
 * Copyright © 2013 Jason Ekstrand
 *
 * Permission is hereby granted, free of charge, to any person obtaining
 * a copy of this software and associated documentation files (the
 * "Software"), to deal in the Software without restriction, including
 * without limitation the rights to use, copy, modify, merge, publish,
 * distribute, sublicense, and/or sell copies of the Software, and to
 * permit persons to whom the Software is furnished to do so, subject to
 * the following conditions:
 *
 * The above copyright notice and this permission notice (including the
 * next paragraph) shall be included in all copies or substantial
 * portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
 * EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
 * NONINFRINGEMENT.  IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS
 * BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN
 * ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
 * CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 */

#define _GNU_SOURCE

#include "../config.h"

#include <math.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <stdio.h>
#include <errno.h>
#include <sys/uio.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <time.h>
#include <ffi.h>

#include "wayland-util.h"
#include "wayland-private.h"
#include "wayland-os.h"

static inline uint32_t
div_roundup(uint32_t n, size_t a)
{
	/* The cast to uint64_t is necessary to prevent overflow when rounding
	 * values close to UINT32_MAX. After the division it is again safe to
	 * cast back to uint32_t.
	 */
	return (uint32_t) (((uint64_t) n + (a - 1)) / a);
}

struct wl_ring_buffer {
	char *data;
	size_t head, tail;
	uint32_t size_bits;
	uint32_t max_size_bits;  /* 0 for unlimited */
};

#define MAX_FDS_OUT	28
#define CLEN		(CMSG_LEN(MAX_FDS_OUT * sizeof(int32_t)))

struct wl_connection {
	struct wl_ring_buffer in, out;
	struct wl_ring_buffer fds_in, fds_out;
	int fd;
	int want_flush;
};

static inline size_t
size_pot(uint32_t size_bits)
{
	if (!(size_bits < 8 * sizeof(size_t)))
		wl_abort("Too many bits for size_t\n");

	return ((size_t)1) << size_bits;
}

static size_t
ring_buffer_capacity(const struct wl_ring_buffer *b) {
	return size_pot(b->size_bits);
}

static size_t
ring_buffer_mask(const struct wl_ring_buffer *b, size_t i) {
	size_t m = ring_buffer_capacity(b) - 1;
	return i & m;
}

static int
ring_buffer_put(struct wl_ring_buffer *b, const void *data, size_t count)
{
	size_t head, size;

	if (count == 0)
		return 0;

	head = ring_buffer_mask(b, b->head);
	if (head + count <= ring_buffer_capacity(b)) {
		memcpy(b->data + head, data, count);
	} else {
		size = ring_buffer_capacity(b) - head;
		memcpy(b->data + head, data, size);
		memcpy(b->data, (const char *) data + size, count - size);
	}

	b->head += count;

	return 0;
}

static void
ring_buffer_put_iov(struct wl_ring_buffer *b, struct iovec *iov, int *count)
{
	size_t head, tail;

	head = ring_buffer_mask(b, b->head);
	tail = ring_buffer_mask(b, b->tail);
	if (head < tail) {
		iov[0].iov_base = b->data + head;
		iov[0].iov_len = tail - head;
		*count = 1;
	} else if (tail == 0) {
		iov[0].iov_base = b->data + head;
		iov[0].iov_len = ring_buffer_capacity(b) - head;
		*count = 1;
	} else {
		iov[0].iov_base = b->data + head;
		iov[0].iov_len = ring_buffer_capacity(b) - head;
		iov[1].iov_base = b->data;
		iov[1].iov_len = tail;
		*count = 2;
	}
}

static void
ring_buffer_get_iov(struct wl_ring_buffer *b, struct iovec *iov, int *count)
{
	size_t head, tail;

	head = ring_buffer_mask(b, b->head);
	tail = ring_buffer_mask(b, b->tail);
	if (tail < head) {
		iov[0].iov_base = b->data + tail;
		iov[0].iov_len = head - tail;
		*count = 1;
	} else if (head == 0) {
		iov[0].iov_base = b->data + tail;
		iov[0].iov_len = ring_buffer_capacity(b) - tail;
		*count = 1;
	} else {
		iov[0].iov_base = b->data + tail;
		iov[0].iov_len = ring_buffer_capacity(b) - tail;
		iov[1].iov_base = b->data;
		iov[1].iov_len = head;
		*count = 2;
	}
}

static void
ring_buffer_copy(struct wl_ring_buffer *b, void *data, size_t count)
{
	size_t tail, size;

	if (count == 0)
		return;

	tail = ring_buffer_mask(b, b->tail);
	if (tail + count <= ring_buffer_capacity(b)) {
		memcpy(data, b->data + tail, count);
	} else {
		size = ring_buffer_capacity(b) - tail;
		memcpy(data, b->data + tail, size);
		memcpy((char *) data + size, b->data, count - size);
	}
}

static size_t
ring_buffer_size(struct wl_ring_buffer *b)
{
	return b->head - b->tail;
}

static char *
ring_buffer_tail(const struct wl_ring_buffer *b)
{
	return b->data + ring_buffer_mask(b, b->tail);
}

static uint32_t
get_max_size_bits_for_size(size_t buffer_size)
{
	uint32_t max_size_bits = WL_BUFFER_DEFAULT_SIZE_POT;

	/* buffer_size == 0 means unbound buffer size */
	if (buffer_size == 0)
		return 0;

	while (max_size_bits < 8 * sizeof(size_t) && size_pot(max_size_bits) < buffer_size)
		max_size_bits++;

	return max_size_bits;
}

static int
ring_buffer_allocate(struct wl_ring_buffer *b, size_t size_bits)
{
	char *new_data;

	new_data = calloc(size_pot(size_bits), 1);
	if (!new_data)
		return -1;

	ring_buffer_copy(b, new_data, ring_buffer_size(b));
	free(b->data);
	b->data = new_data;
	b->size_bits = size_bits;
	b->head = ring_buffer_size(b);
	b->tail = 0;

	return 0;
}

static size_t
ring_buffer_get_bits_for_size(struct wl_ring_buffer *b, size_t net_size)
{
	size_t max_size_bits = get_max_size_bits_for_size(net_size);

	if (max_size_bits < WL_BUFFER_DEFAULT_SIZE_POT)
		max_size_bits = WL_BUFFER_DEFAULT_SIZE_POT;

	if (b->max_size_bits > 0 && max_size_bits > b->max_size_bits)
		max_size_bits = b->max_size_bits;

	return max_size_bits;
}

static bool
ring_buffer_is_max_size_reached(struct wl_ring_buffer *b)
{
	size_t net_size = ring_buffer_size(b) + 1;
	size_t size_bits = ring_buffer_get_bits_for_size(b, net_size);

	return net_size >= size_pot(size_bits);
}

static int
ring_buffer_ensure_space(struct wl_ring_buffer *b, size_t count)
{
	size_t net_size = ring_buffer_size(b) + count;
	size_t size_bits = ring_buffer_get_bits_for_size(b, net_size);

	/* The 'size_bits' value represents the required size (in POT) to store
	 * 'net_size', which depending whether the buffers are bounded or not
	 * might not be sufficient (i.e. we might have reached the maximum size
	 * allowed).
	 */
	if (net_size > size_pot(size_bits)) {
		wl_log("Data too big for buffer (%zu + %zu > %zu).\n",
		       ring_buffer_size(b), count, size_pot(size_bits));
		errno = E2BIG;
		return -1;
	}

	/* The following test here is a short-cut to avoid reallocating a buffer
	 * of the same size.
	 */
	if (size_bits == b->size_bits)
		return 0;

	/* Otherwise, we (re)allocate the buffer to match the required size */
	return ring_buffer_allocate(b, size_bits);
}

static void
ring_buffer_close_fds(struct wl_ring_buffer *buffer, int32_t count)
{
	int32_t i, *p;
	size_t size, tail;

	size = ring_buffer_capacity(buffer);
	tail = ring_buffer_mask(buffer, buffer->tail);
	p = (int32_t *) (buffer->data + tail);

	for (i = 0; i < count; i++) {
		if (p >= (int32_t *) (buffer->data + size))
			p = (int32_t *) buffer->data;
		close(*p++);
	}
}

void
wl_connection_set_max_buffer_size(struct wl_connection *connection,
				  size_t max_buffer_size)
{
	uint32_t max_size_bits;

	max_size_bits = get_max_size_bits_for_size(max_buffer_size);

	connection->fds_in.max_size_bits = max_size_bits;
	ring_buffer_ensure_space(&connection->fds_in, 0);

	connection->fds_out.max_size_bits = max_size_bits;
	ring_buffer_ensure_space(&connection->fds_out, 0);

	connection->in.max_size_bits = max_size_bits;
	ring_buffer_ensure_space(&connection->in, 0);

	connection->out.max_size_bits = max_size_bits;
	ring_buffer_ensure_space(&connection->out, 0);
}

struct wl_connection *
wl_connection_create(int fd, size_t max_buffer_size)
{
	struct wl_connection *connection;

	connection = zalloc(sizeof *connection);
	if (connection == NULL)
		return NULL;

	wl_connection_set_max_buffer_size(connection, max_buffer_size);

	connection->fd = fd;

	return connection;
}

static void
close_fds(struct wl_ring_buffer *buffer, int max)
{
	size_t size;
	int32_t count;

	size = ring_buffer_size(buffer);
	if (size == 0)
		return;

	count = size / sizeof(int32_t);
	if (max > 0 && max < count)
		count = max;

	ring_buffer_close_fds(buffer, count);

	size = count * sizeof(int32_t);
	buffer->tail += size;
}

void
wl_connection_close_fds_in(struct wl_connection *connection, int max)
{
	close_fds(&connection->fds_in, max);
}

int
wl_connection_destroy(struct wl_connection *connection)
{
	int fd = connection->fd;

	close_fds(&connection->fds_out, -1);
	free(connection->fds_out.data);
	free(connection->out.data);

	close_fds(&connection->fds_in, -1);
	free(connection->fds_in.data);
	free(connection->in.data);

	free(connection);

	return fd;
}

void
wl_connection_copy(struct wl_connection *connection, void *data, size_t size)
{
	ring_buffer_copy(&connection->in, data, size);
}

void
wl_connection_consume(struct wl_connection *connection, size_t size)
{
	connection->in.tail += size;
}

static void
build_cmsg(struct wl_ring_buffer *buffer, char *data, size_t *clen)
{
	struct cmsghdr *cmsg;
	size_t size;

	size = ring_buffer_size(buffer);
	if (size > MAX_FDS_OUT * sizeof(int32_t))
		size = MAX_FDS_OUT * sizeof(int32_t);

	if (size > 0) {
		cmsg = (struct cmsghdr *) data;
		cmsg->cmsg_level = SOL_SOCKET;
		cmsg->cmsg_type = SCM_RIGHTS;
		cmsg->cmsg_len = CMSG_LEN(size);
		ring_buffer_copy(buffer, CMSG_DATA(cmsg), size);
		*clen = cmsg->cmsg_len;
	} else {
		*clen = 0;
	}
}

static int
decode_cmsg(struct wl_ring_buffer *buffer, struct msghdr *msg)
{
	struct cmsghdr *cmsg;
	size_t size, i;
	int overflow = 0;

	for (cmsg = CMSG_FIRSTHDR(msg); cmsg != NULL;
	     cmsg = CMSG_NXTHDR(msg, cmsg)) {
		if (cmsg->cmsg_level != SOL_SOCKET ||
		    cmsg->cmsg_type != SCM_RIGHTS)
			continue;

		size = cmsg->cmsg_len - CMSG_LEN(0);

		if (ring_buffer_ensure_space(buffer, size) < 0 || overflow) {
			overflow = 1;
			size /= sizeof(int32_t);
			for (i = 0; i < size; i++)
				close(((int*)CMSG_DATA(cmsg))[i]);
		} else if (ring_buffer_put(buffer, CMSG_DATA(cmsg), size) < 0) {
				return -1;
		}
	}

	if (overflow) {
		errno = EOVERFLOW;
		return -1;
	}

	return 0;
}

int
wl_connection_flush(struct wl_connection *connection)
{
	struct iovec iov[2];
	struct msghdr msg = {0};
	char cmsg[CLEN];
	int len = 0, count;
	size_t clen;
	size_t tail;

	if (!connection->want_flush)
		return 0;

	tail = connection->out.tail;
	while (ring_buffer_size(&connection->out) > 0) {
		build_cmsg(&connection->fds_out, cmsg, &clen);

		if (clen >= CLEN) {
			/* UNIX domain sockets allows to send file descriptors
			 * using ancillary data.
			 *
			 * As per the UNIX domain sockets man page (man 7 unix),
			 * "at least one byte of real data should be sent when
			 * sending ancillary data".
			 *
			 * This is why we send only a single byte here, to ensure
			 * all file descriptors are sent before the bytes are
			 * cleared out.
			 *
			 * Otherwise This can fail to clear the file descriptors
			 * first if individual messages are allowed to have 224
			 * (8 bytes * MAX_FDS_OUT = 224) file descriptors .
			 */
			iov[0].iov_base = ring_buffer_tail(&connection->out);
			iov[0].iov_len = 1;
			count = 1;
		} else {
			ring_buffer_get_iov(&connection->out, iov, &count);
		}

		msg.msg_name = NULL;
		msg.msg_namelen = 0;
		msg.msg_iov = iov;
		msg.msg_iovlen = count;
		msg.msg_control = (clen > 0) ? cmsg : NULL;
		msg.msg_controllen = clen;

		do {
			len = sendmsg(connection->fd, &msg,
				      MSG_NOSIGNAL | MSG_DONTWAIT);
		} while (len == -1 && errno == EINTR);

		if (len == -1)
			return -1;

		close_fds(&connection->fds_out, MAX_FDS_OUT);

		connection->out.tail += len;
	}

	connection->want_flush = 0;

	return connection->out.head - tail;
}

uint32_t
wl_connection_pending_input(struct wl_connection *connection)
{
	return ring_buffer_size(&connection->in);
}

int
wl_connection_read(struct wl_connection *connection)
{
	struct iovec iov[2];
	struct msghdr msg;
	char cmsg[CLEN];
	int len, count, ret;

	while (1) {
		int data_size = ring_buffer_size(&connection->in);

		/* Stop once we've read the max buffer size. */
		if (ring_buffer_is_max_size_reached(&connection->in))
			return data_size;

		if (ring_buffer_ensure_space(&connection->in, 1) < 0)
			return -1;

		ring_buffer_put_iov(&connection->in, iov, &count);

		msg.msg_name = NULL;
		msg.msg_namelen = 0;
		msg.msg_iov = iov;
		msg.msg_iovlen = count;
		msg.msg_control = cmsg;
		msg.msg_controllen = sizeof cmsg;
		msg.msg_flags = 0;

		do {
			len = wl_os_recvmsg_cloexec(connection->fd, &msg, MSG_DONTWAIT);
		} while (len < 0 && errno == EINTR);

		if (len == 0) {
		    /* EOF, return previously read data first */
		    return data_size;
		}
		if (len < 0) {
		    if (errno == EAGAIN && data_size > 0) {
			/* nothing new read, return previously read data */
			return data_size;
		    }
		    return len;
		}

		ret = decode_cmsg(&connection->fds_in, &msg);
		if (ret)
			return -1;

		connection->in.head += len;
	}
}

int
wl_connection_write(struct wl_connection *connection,
		    const void *data, size_t count)
{
	if (wl_connection_queue(connection, data, count) < 0)
		return -1;

	connection->want_flush = 1;

	return 0;
}

int
wl_connection_queue(struct wl_connection *connection,
		    const void *data, size_t count)
{
	/* We want to try to flush when the buffer reaches the default maximum
	 * size even if the buffer has been previously expanded.
	 *
	 * Otherwise the larger buffer will cause us to flush less frequently,
	 * which could increase lag.
	 *
	 * We'd like to flush often and get the buffer size back down if possible.
	 */
	if (ring_buffer_size(&connection->out) + count > WL_BUFFER_DEFAULT_MAX_SIZE) {
		connection->want_flush = 1;
		if (wl_connection_flush(connection) < 0 && errno != EAGAIN)
			return -1;
	}

	if (ring_buffer_ensure_space(&connection->out, count) < 0)
		return -1;

	return ring_buffer_put(&connection->out, data, count);
}

int
wl_message_count_arrays(const struct wl_message *message)
{
	int i, arrays;

	for (i = 0, arrays = 0; message->signature[i]; i++) {
		if (message->signature[i] == WL_ARG_ARRAY)
			arrays++;
	}

	return arrays;
}

int
wl_connection_get_fd(struct wl_connection *connection)
{
	return connection->fd;
}

static int
wl_connection_put_fd(struct wl_connection *connection, int32_t fd)
{
	if (ring_buffer_size(&connection->fds_out) >= MAX_FDS_OUT * sizeof fd) {
		connection->want_flush = 1;
		if (wl_connection_flush(connection) < 0 && errno != EAGAIN)
			return -1;
	}

	if (ring_buffer_ensure_space(&connection->fds_out, sizeof fd) < 0)
		return -1;

	return ring_buffer_put(&connection->fds_out, &fd, sizeof fd);
}

const char *
get_next_argument(const char *signature, struct argument_details *details)
{
	details->nullable = 0;
	for(; *signature; ++signature) {
		switch(*signature) {
		case WL_ARG_INT:
		case WL_ARG_UINT:
		case WL_ARG_FIXED:
		case WL_ARG_STRING:
		case WL_ARG_OBJECT:
		case WL_ARG_NEW_ID:
		case WL_ARG_ARRAY:
		case WL_ARG_FD:
			details->type = *signature;
			return signature + 1;
		case '?':
			details->nullable = 1;
		}
	}
	details->type = '\0';
	return signature;
}

int
arg_count_for_signature(const char *signature)
{
	int count = 0;
	for(; *signature; ++signature) {
		switch(*signature) {
		case WL_ARG_INT:
		case WL_ARG_UINT:
		case WL_ARG_FIXED:
		case WL_ARG_STRING:
		case WL_ARG_OBJECT:
		case WL_ARG_NEW_ID:
		case WL_ARG_ARRAY:
		case WL_ARG_FD:
			++count;
		}
	}
	return count;
}

int
wl_message_get_since(const struct wl_message *message)
{
	int since;

	since = atoi(message->signature);

	if (since == 0)
		since = 1;

	return since;
}

void
wl_argument_from_va_list(const char *signature, union wl_argument *args,
			 int count, va_list ap)
{
	int i;
	const char *sig_iter;
	struct argument_details arg;

	sig_iter = signature;
	for (i = 0; i < count; i++) {
		sig_iter = get_next_argument(sig_iter, &arg);

		switch(arg.type) {
		case WL_ARG_INT:
			args[i].i = va_arg(ap, int32_t);
			break;
		case WL_ARG_UINT:
			args[i].u = va_arg(ap, uint32_t);
			break;
		case WL_ARG_FIXED:
			args[i].f = va_arg(ap, wl_fixed_t);
			break;
		case WL_ARG_STRING:
			args[i].s = va_arg(ap, const char *);
			break;
		case WL_ARG_OBJECT:
			args[i].o = va_arg(ap, struct wl_object *);
			break;
		case WL_ARG_NEW_ID:
			args[i].o = va_arg(ap, struct wl_object *);
			break;
		case WL_ARG_ARRAY:
			args[i].a = va_arg(ap, struct wl_array *);
			break;
		case WL_ARG_FD:
			args[i].h = va_arg(ap, int32_t);
			break;
		}
	}
}

static void
wl_closure_clear_fds(struct wl_closure *closure)
{
	const char *signature = closure->message->signature;
	struct argument_details arg;
	int i;

	for (i = 0; i < closure->count; i++) {
		signature = get_next_argument(signature, &arg);
		if (arg.type == WL_ARG_FD)
			closure->args[i].h = -1;
	}
}

static struct wl_closure *
wl_closure_init(const struct wl_message *message, uint32_t size,
                int *num_arrays, union wl_argument *args)
{
	struct wl_closure *closure;
	int count;

	count = arg_count_for_signature(message->signature);
	if (count > WL_CLOSURE_MAX_ARGS) {
		wl_log("too many args (%d) for %s (signature %s)\n", count,
		       message->name, message->signature);
		errno = EINVAL;
		return NULL;
	}

	int size_to_allocate;

	if (size) {
		*num_arrays = wl_message_count_arrays(message);
		size_to_allocate = sizeof *closure + size +
				   *num_arrays * sizeof(struct wl_array);
	} else {
		size_to_allocate = sizeof *closure;
	}
	closure = zalloc(size_to_allocate);

	if (!closure) {
		wl_log("could not allocate closure of size (%d) for "
		       "%s (signature %s)\n", size_to_allocate, message->name,
		       message->signature);
		errno = ENOMEM;
		return NULL;
	}

	if (args)
		memcpy(closure->args, args, count * sizeof *args);

	closure->message = message;
	closure->count = count;

	/* Set these all to -1 so we can close any that have been
	 * set to a real value during wl_closure_destroy().
	 * We may have copied a bunch of fds into the closure with
	 * memcpy previously, but those are undup()d client fds
	 * that we would have replaced anyway.
	 */
	wl_closure_clear_fds(closure);

	return closure;
}

struct wl_closure *
wl_closure_marshal(struct wl_object *sender, uint32_t opcode,
		   union wl_argument *args,
		   const struct wl_message *message)
{
	struct wl_closure *closure;
	struct wl_object *object;
	int i, count, fd, dup_fd;
	const char *signature;
	struct argument_details arg;

	closure = wl_closure_init(message, 0, NULL, args);
	if (closure == NULL)
		return NULL;

	count = closure->count;

	signature = message->signature;
	for (i = 0; i < count; i++) {
		signature = get_next_argument(signature, &arg);

		switch (arg.type) {
		case WL_ARG_FIXED:
		case WL_ARG_UINT:
		case WL_ARG_INT:
			break;
		case WL_ARG_STRING:
			if (!arg.nullable && args[i].s == NULL)
				goto err_null;
			break;
		case WL_ARG_OBJECT:
			if (!arg.nullable && args[i].o == NULL)
				goto err_null;
			break;
		case WL_ARG_NEW_ID:
			object = args[i].o;
			if (object == NULL)
				goto err_null;

			closure->args[i].n = object ? object->id : 0;
			break;
		case WL_ARG_ARRAY:
			if (args[i].a == NULL)
				goto err_null;
			break;
		case WL_ARG_FD:
			fd = args[i].h;
			dup_fd = wl_os_dupfd_cloexec(fd, 0);
			if (dup_fd < 0) {
				wl_closure_destroy(closure);
				wl_log("error marshalling arguments for %s: dup failed: %s\n",
				       message->name, strerror(errno));
				return NULL;
			}
			closure->args[i].h = dup_fd;
			break;
		default:
			wl_abort("unhandled format code: '%c'\n", arg.type);
			break;
		}
	}

	closure->sender_id = sender->id;
	closure->opcode = opcode;

	return closure;

err_null:
	wl_closure_destroy(closure);
	wl_log("error marshalling arguments for %s (signature %s): "
	       "null value passed for arg %i\n", message->name,
	       message->signature, i);
	errno = EINVAL;
	return NULL;
}

struct wl_closure *
wl_closure_vmarshal(struct wl_object *sender, uint32_t opcode, va_list ap,
		    const struct wl_message *message)
{
	union wl_argument args[WL_CLOSURE_MAX_ARGS];

	wl_argument_from_va_list(message->signature, args,
				 WL_CLOSURE_MAX_ARGS, ap);

	return wl_closure_marshal(sender, opcode, args, message);
}

struct wl_closure *
wl_connection_demarshal(struct wl_connection *connection,
			uint32_t size,
			struct wl_map *objects,
			const struct wl_message *message)
{
	uint32_t *p, *next, *end, length, length_in_u32, id;
	int fd;
	char *s;
	int i, count, num_arrays;
	const char *signature;
	struct argument_details arg;
	struct wl_closure *closure;
	struct wl_array *array_extra;

	/* Space for sender_id and opcode */
	if (size < 2 * sizeof *p) {
		wl_log("message too short, invalid header\n");
		wl_connection_consume(connection, size);
		errno = EINVAL;
		return NULL;
	}

	closure = wl_closure_init(message, size, &num_arrays, NULL);
	if (closure == NULL) {
		wl_connection_consume(connection, size);
		return NULL;
	}

	count = closure->count;

	array_extra = closure->extra;
	p = (uint32_t *)(closure->extra + num_arrays);
	end = p + size / sizeof *p;

	wl_connection_copy(connection, p, size);
	closure->sender_id = *p++;
	closure->opcode = *p++ & 0x0000ffff;

	signature = message->signature;
	for (i = 0; i < count; i++) {
		signature = get_next_argument(signature, &arg);

		if (arg.type != WL_ARG_FD && p >= end) {
			wl_log("message too short, "
			       "object (%d), message %s(%s)\n",
			       closure->sender_id, message->name,
			       message->signature);
			errno = EINVAL;
			goto err;
		}

		switch (arg.type) {
		case WL_ARG_UINT:
			closure->args[i].u = *p++;
			break;
		case WL_ARG_INT:
			closure->args[i].i = *p++;
			break;
		case WL_ARG_FIXED:
			closure->args[i].f = *p++;
			break;
		case WL_ARG_STRING:
			length = *p++;

			if (length == 0 && !arg.nullable) {
				wl_log("NULL string received on non-nullable "
				       "type, message %s(%s)\n", message->name,
				       message->signature);
				errno = EINVAL;
				goto err;
			}
			if (length == 0) {
				closure->args[i].s = NULL;
				break;
			}

			length_in_u32 = div_roundup(length, sizeof *p);
			if ((uint32_t) (end - p) < length_in_u32) {
				wl_log("message too short, "
				       "object (%d), message %s(%s)\n",
				       closure->sender_id, message->name,
				       message->signature);
				errno = EINVAL;
				goto err;
			}
			next = p + length_in_u32;

			s = (char *) p;

			if (s[length - 1] != '\0') {
				wl_log("string not nul-terminated, "
				       "message %s(%s)\n",
				       message->name, message->signature);
				errno = EINVAL;
				goto err;
			}

			if (strlen(s) != length - 1) {
				wl_log("string has embedded nul at offset %zu, "
				       "message %s(%s)\n", strlen(s),
				       message->name, message->signature);
				errno = EINVAL;
				goto err;
			}

			closure->args[i].s = s;
			p = next;
			break;
		case WL_ARG_OBJECT:
			id = *p++;
			closure->args[i].n = id;

			if (id == 0 && !arg.nullable) {
				wl_log("NULL object received on non-nullable "
				       "type, message %s(%s)\n", message->name,
				       message->signature);
				errno = EINVAL;
				goto err;
			}
			break;
		case WL_ARG_NEW_ID:
			id = *p++;
			closure->args[i].n = id;

			if (id == 0) {
				wl_log("NULL new ID received on non-nullable "
				       "type, message %s(%s)\n", message->name,
				       message->signature);
				errno = EINVAL;
				goto err;
			}

			if (wl_map_reserve_new(objects, id) < 0) {
				if (errno == EINVAL) {
					wl_log("not a valid new object id (%u), "
					       "message %s(%s)\n", id,
					       message->name,
					       message->signature);
				}
				goto err;
			}

			break;
		case WL_ARG_ARRAY:
			length = *p++;

			length_in_u32 = div_roundup(length, sizeof *p);
			if ((uint32_t) (end - p) < length_in_u32) {
				wl_log("message too short, "
				       "object (%d), message %s(%s)\n",
				       closure->sender_id, message->name,
				       message->signature);
				errno = EINVAL;
				goto err;
			}
			next = p + length_in_u32;

			array_extra->size = length;
			array_extra->alloc = 0;
			array_extra->data = p;

			closure->args[i].a = array_extra++;
			p = next;
			break;
		case WL_ARG_FD:
			if (connection->fds_in.tail == connection->fds_in.head) {
				wl_log("file descriptor expected, "
				       "object (%d), message %s(%s)\n",
				       closure->sender_id, message->name,
				       message->signature);
				errno = EINVAL;
				goto err;
			}

			ring_buffer_copy(&connection->fds_in, &fd, sizeof fd);
			connection->fds_in.tail += sizeof fd;
			closure->args[i].h = fd;
			break;
		default:
			wl_abort("unknown type\n");
			break;
		}
	}

	wl_connection_consume(connection, size);

	return closure;

 err:
	wl_closure_destroy(closure);
	wl_connection_consume(connection, size);

	return NULL;
}

bool
wl_object_is_zombie(struct wl_map *map, uint32_t id)
{
	uint32_t flags;

	/* Zombie objects only exist on the client side. */
	if (map->side == WL_MAP_SERVER_SIDE)
		return false;

	/* Zombie objects can only have been created by the client. */
	if (id >= WL_SERVER_ID_START)
		return false;

	flags = wl_map_lookup_flags(map, id);
	return !!(flags & WL_MAP_ENTRY_ZOMBIE);
}

int
wl_closure_lookup_objects(struct wl_closure *closure, struct wl_map *objects)
{
	struct wl_object *object;
	const struct wl_message *message;
	const char *signature;
	struct argument_details arg;
	int i, count;
	uint32_t id;

	message = closure->message;
	signature = message->signature;
	count = arg_count_for_signature(signature);
	for (i = 0; i < count; i++) {
		signature = get_next_argument(signature, &arg);
		if (arg.type != WL_ARG_OBJECT)
			continue;

		id = closure->args[i].n;
		closure->args[i].o = NULL;

		object = wl_map_lookup(objects, id);
		if (wl_object_is_zombie(objects, id)) {
			/* references object we've already
			 * destroyed client side */
			object = NULL;
		} else if (object == NULL && id != 0) {
			wl_log("unknown object (%u), message %s(%s)\n",
			       id, message->name, message->signature);
			errno = EINVAL;
			return -1;
		}

		if (object != NULL && message->types[i] != NULL &&
		    !wl_interface_equal((object)->interface,
					message->types[i])) {
			wl_log("invalid object (%u), type (%s), "
			       "message %s(%s)\n",
			       id, (object)->interface->name,
			       message->name, message->signature);
			errno = EINVAL;
			return -1;
		}
		closure->args[i].o = object;
	}

	return 0;
}

static void
convert_arguments_to_ffi(const char *signature, uint32_t flags,
			 union wl_argument *args,
			 int count, ffi_type **ffi_types, void** ffi_args)
{
	int i;
	const char *sig_iter;
	struct argument_details arg;

	sig_iter = signature;
	for (i = 0; i < count; i++) {
		sig_iter = get_next_argument(sig_iter, &arg);

		switch(arg.type) {
		case WL_ARG_INT:
			ffi_types[i] = &ffi_type_sint32;
			ffi_args[i] = &args[i].i;
			break;
		case WL_ARG_UINT:
			ffi_types[i] = &ffi_type_uint32;
			ffi_args[i] = &args[i].u;
			break;
		case WL_ARG_FIXED:
			ffi_types[i] = &ffi_type_sint32;
			ffi_args[i] = &args[i].f;
			break;
		case WL_ARG_STRING:
			ffi_types[i] = &ffi_type_pointer;
			ffi_args[i] = &args[i].s;
			break;
		case WL_ARG_OBJECT:
			ffi_types[i] = &ffi_type_pointer;
			ffi_args[i] = &args[i].o;
			break;
		case WL_ARG_NEW_ID:
			if (flags & WL_CLOSURE_INVOKE_CLIENT) {
				ffi_types[i] = &ffi_type_pointer;
				ffi_args[i] = &args[i].o;
			} else {
				ffi_types[i] = &ffi_type_uint32;
				ffi_args[i] = &args[i].n;
			}
			break;
		case WL_ARG_ARRAY:
			ffi_types[i] = &ffi_type_pointer;
			ffi_args[i] = &args[i].a;
			break;
		case WL_ARG_FD:
			ffi_types[i] = &ffi_type_sint32;
			ffi_args[i] = &args[i].h;
			break;
		default:
			wl_abort("unknown type\n");
			break;
		}
	}
}

void
wl_closure_invoke(struct wl_closure *closure, uint32_t flags,
		  struct wl_object *target, uint32_t opcode, void *data)
{
	int count;
	ffi_cif cif;
	ffi_type *ffi_types[WL_CLOSURE_MAX_ARGS + 2];
	void * ffi_args[WL_CLOSURE_MAX_ARGS + 2];
	void (* const *implementation)(void);

	count = arg_count_for_signature(closure->message->signature);

	ffi_types[0] = &ffi_type_pointer;
	ffi_args[0] = &data;
	ffi_types[1] = &ffi_type_pointer;
	ffi_args[1] = &target;

	convert_arguments_to_ffi(closure->message->signature, flags, closure->args,
				 count, ffi_types + 2, ffi_args + 2);

	ffi_prep_cif(&cif, FFI_DEFAULT_ABI,
		     count + 2, &ffi_type_void, ffi_types);

	implementation = target->implementation;
	if (!implementation) {
		wl_abort("Implementation of resource %d of %s is NULL\n",
			  target->id, target->interface->name);
	}

	if (!implementation[opcode]) {
		wl_abort("listener function for opcode %u of %s is NULL\n",
			 opcode, target->interface->name);
	}
	ffi_call(&cif, implementation[opcode], NULL, ffi_args);

	wl_closure_clear_fds(closure);
}

void
wl_closure_dispatch(struct wl_closure *closure, wl_dispatcher_func_t dispatcher,
		    struct wl_object *target, uint32_t opcode)
{
	dispatcher(target->implementation, target, opcode, closure->message,
		   closure->args);

	wl_closure_clear_fds(closure);
}

static int
copy_fds_to_connection(struct wl_closure *closure,
		       struct wl_connection *connection)
{
	const struct wl_message *message = closure->message;
	uint32_t i, count;
	struct argument_details arg;
	const char *signature = message->signature;
	int fd;

	count = arg_count_for_signature(signature);
	for (i = 0; i < count; i++) {
		signature = get_next_argument(signature, &arg);
		if (arg.type != WL_ARG_FD)
			continue;

		fd = closure->args[i].h;
		if (wl_connection_put_fd(connection, fd)) {
			wl_log("request could not be marshaled: "
			       "can't send file descriptor\n");
			return -1;
		}
		closure->args[i].h = -1;
	}

	return 0;
}


static uint32_t
buffer_size_for_closure(struct wl_closure *closure)
{
	const struct wl_message *message = closure->message;
	int i, count;
	struct argument_details arg;
	const char *signature;
	uint32_t size, buffer_size = 0;

	signature = message->signature;
	count = arg_count_for_signature(signature);
	for (i = 0; i < count; i++) {
		signature = get_next_argument(signature, &arg);

		switch (arg.type) {
		case WL_ARG_FD:
			break;
		case WL_ARG_UINT:
		case WL_ARG_INT:
		case WL_ARG_FIXED:
		case WL_ARG_OBJECT:
		case WL_ARG_NEW_ID:
			buffer_size++;
			break;
		case WL_ARG_STRING:
			if (closure->args[i].s == NULL) {
				buffer_size++;
				break;
			}

			size = strlen(closure->args[i].s) + 1;
			buffer_size += 1 + div_roundup(size, sizeof(uint32_t));
			break;
		case WL_ARG_ARRAY:
			if (closure->args[i].a == NULL) {
				buffer_size++;
				break;
			}

			size = closure->args[i].a->size;
			buffer_size += (1 + div_roundup(size, sizeof(uint32_t)));
			break;
		default:
			break;
		}
	}

	return buffer_size + 2;
}

static int
serialize_closure(struct wl_closure *closure, uint32_t *buffer,
		  size_t buffer_count)
{
	const struct wl_message *message = closure->message;
	unsigned int i, count, size;
	uint32_t *p, *end;
	struct argument_details arg;
	const char *signature;

	if (buffer_count < 2)
		goto overflow;

	p = buffer + 2;
	end = buffer + buffer_count;

	signature = message->signature;
	count = arg_count_for_signature(signature);
	for (i = 0; i < count; i++) {
		signature = get_next_argument(signature, &arg);

		if (arg.type == WL_ARG_FD)
			continue;

		if (p >= end)
			goto overflow;

		switch (arg.type) {
		case WL_ARG_UINT:
			*p++ = closure->args[i].u;
			break;
		case WL_ARG_INT:
			*p++ = closure->args[i].i;
			break;
		case WL_ARG_FIXED:
			*p++ = closure->args[i].f;
			break;
		case WL_ARG_OBJECT:
			*p++ = closure->args[i].o ? closure->args[i].o->id : 0;
			break;
		case WL_ARG_NEW_ID:
			*p++ = closure->args[i].n;
			break;
		case WL_ARG_STRING:
			if (closure->args[i].s == NULL) {
				*p++ = 0;
				break;
			}

			size = strlen(closure->args[i].s) + 1;
			*p++ = size;

			if (div_roundup(size, sizeof *p) > (uint32_t)(end - p))
				goto overflow;

			memcpy(p, closure->args[i].s, size);
			p += div_roundup(size, sizeof *p);
			break;
		case WL_ARG_ARRAY:
			if (closure->args[i].a == NULL) {
				*p++ = 0;
				break;
			}

			size = closure->args[i].a->size;
			*p++ = size;

			if (div_roundup(size, sizeof *p) > (uint32_t)(end - p))
				goto overflow;

			if (size != 0)
				memcpy(p, closure->args[i].a->data, size);
			p += div_roundup(size, sizeof *p);
			break;
		case WL_ARG_FD:
			break;
		}
	}

	size = (p - buffer) * sizeof *p;

	buffer[0] = closure->sender_id;
	buffer[1] = size << 16 | (closure->opcode & 0x0000ffff);

	return size;

overflow:
	wl_log("serialize_closure overflow for %s (signature %s)\n",
	       message->name, message->signature);
	errno = ERANGE;
	return -1;
}

int
wl_closure_send(struct wl_closure *closure, struct wl_connection *connection)
{
	int size;
	uint32_t buffer_size;
	uint32_t *buffer;
	int result;

	if (copy_fds_to_connection(closure, connection))
		return -1;

	buffer_size = buffer_size_for_closure(closure);
	buffer = zalloc(buffer_size * sizeof buffer[0]);
	if (buffer == NULL) {
		wl_log("wl_closure_send error: buffer allocation failure of "
		       "size %d\n for %s (signature %s)",
		       buffer_size * sizeof buffer[0], closure->message->name,
		       closure->message->signature);
		return -1;
	}

	size = serialize_closure(closure, buffer, buffer_size);
	if (size < 0) {
		free(buffer);
		return -1;
	}

	result = wl_connection_write(connection, buffer, size);
	free(buffer);

	return result;
}

int
wl_closure_queue(struct wl_closure *closure, struct wl_connection *connection)
{
	int size;
	uint32_t buffer_size;
	uint32_t *buffer;
	int result;

	if (copy_fds_to_connection(closure, connection))
		return -1;

	buffer_size = buffer_size_for_closure(closure);
	buffer = malloc(buffer_size * sizeof buffer[0]);
	if (buffer == NULL) {
		wl_log("wl_closure_queue error: buffer allocation failure of "
		       "size %d\n for %s (signature %s)",
		       buffer_size * sizeof buffer[0], closure->message->name,
		       closure->message->signature);
		return -1;
	}

	size = serialize_closure(closure, buffer, buffer_size);
	if (size < 0) {
		free(buffer);
		return -1;
	}

	result = wl_connection_queue(connection, buffer, size);
	free(buffer);

	return result;
}

bool
wl_check_env_token(const char *env, const char *token)
{
	const char *ptr = env;
	size_t token_len;

	if (env == NULL)
		return false;

	token_len = strlen(token);

	// Scan the string for comma-separated tokens and look for a match.
	while (true) {
		const char *end;
		size_t len;

		// Skip over any leading separators.
		while (*ptr == ',')
			ptr++;

		if (*ptr == '\x00')
			return false;

		end = strchr(ptr + 1, ',');

		// If there isn't another separarator, then the rest of the string
		// is one token.
		if (end == NULL)
			return (strcmp(ptr, token) == 0);

		len = end - ptr;
		if (len == token_len && memcmp(ptr, token, len) == 0) {
			return true;
		}

		// Skip to the next token.
		ptr += len;
	}

	return false;
}

void
wl_closure_print(struct wl_closure *closure, struct wl_object *target,
		 int send, int discarded, uint32_t (*n_parse)(union wl_argument *arg),
		 const char *queue_name, int color)
{
#if defined(HAVE_GETTID)
	static int include_tid = -1;
#endif // defined(HAVE_GETTID)
	int i;
	struct argument_details arg;
	const char *signature = closure->message->signature;
	struct timespec tp;
	unsigned int time;
	uint32_t nval;
	FILE *f;
	char *buffer;
	size_t buffer_length;

	f = open_memstream(&buffer, &buffer_length);
	if (f == NULL)
		return;

	clock_gettime(CLOCK_REALTIME, &tp);
	time = (tp.tv_sec * 1000000L) + (tp.tv_nsec / 1000);
	fprintf(f, "%s[%7u.%03u] ",
		color ? WL_DEBUG_COLOR_GREEN : "",
		time / 1000, time % 1000);

#if defined(HAVE_GETTID)
	if (include_tid < 0) {
		include_tid = wl_check_env_token(getenv("WAYLAND_DEBUG"), "thread_id");
	}

	if (include_tid) {
		fprintf(f, "%sTID#%d ",
			color ? WL_DEBUG_COLOR_CYAN : "",
			(int) gettid());
	}
#endif

	if (queue_name) {
		fprintf(f, "%s{%s} ",
			color ? WL_DEBUG_COLOR_YELLOW : "",
			queue_name);
	}

	fprintf(f, "%s%s%s%s%s%s%s#%u%s.%s%s(",
		color ? WL_DEBUG_COLOR_RED : "",
		discarded ? "discarded " : "",
		color ? WL_DEBUG_COLOR_RESET : "",
		send ? " -> " : "",
		color ? WL_DEBUG_COLOR_BLUE : "",
		target->interface->name,
		color ? WL_DEBUG_COLOR_MAGENTA : "",
		target->id,
		color ? WL_DEBUG_COLOR_CYAN : "",
		closure->message->name,
		color ? WL_DEBUG_COLOR_RESET : "");

	for (i = 0; i < closure->count; i++) {
		signature = get_next_argument(signature, &arg);
		if (i > 0)
			fprintf(f, ", ");

		switch (arg.type) {
		case WL_ARG_UINT:
			fprintf(f, "%u", closure->args[i].u);
			break;
		case WL_ARG_INT:
			fprintf(f, "%d", closure->args[i].i);
			break;
		case WL_ARG_FIXED:
			/* The magic number 390625 is 1e8 / 256 */
			if (closure->args[i].f >= 0) {
				fprintf(f, "%d.%08d",
					closure->args[i].f / 256,
					390625 * (closure->args[i].f % 256));
			} else {

				fprintf(f, "-%d.%08d",
					closure->args[i].f / -256,
					-390625 * (closure->args[i].f % 256));
			}
			break;
		case WL_ARG_STRING:
			if (closure->args[i].s)
				fprintf(f, "\"%s\"", closure->args[i].s);
			else
				fprintf(f, "nil");
			break;
		case WL_ARG_OBJECT:
			if (closure->args[i].o)
				fprintf(f, "%s#%u",
					closure->args[i].o->interface->name,
					closure->args[i].o->id);
			else
				fprintf(f, "nil");
			break;
		case WL_ARG_NEW_ID:
			if (n_parse)
				nval = n_parse(&closure->args[i]);
			else
				nval = closure->args[i].n;

			fprintf(f, "new id %s#",
				(closure->message->types[i]) ?
				 closure->message->types[i]->name :
				  "[unknown]");
			if (nval != 0)
				fprintf(f, "%u", nval);
			else
				fprintf(f, "nil");
			break;
		case WL_ARG_ARRAY:
			fprintf(f, "array[%zu]", closure->args[i].a->size);
			break;
		case WL_ARG_FD:
			fprintf(f, "fd %d", closure->args[i].h);
			break;
		}
	}

	fprintf(f, ")%s\n", color ? WL_DEBUG_COLOR_RESET : "");

	if (fclose(f) == 0) {
		fprintf(stderr, "%s", buffer);
		free(buffer);
	}
}

static int
wl_closure_close_fds(struct wl_closure *closure)
{
	int i;
	struct argument_details arg;
	const char *signature = closure->message->signature;

	for (i = 0; i < closure->count; i++) {
		signature = get_next_argument(signature, &arg);
		if (arg.type == WL_ARG_FD && closure->args[i].h != -1)
			close(closure->args[i].h);
	}

	return 0;
}

void
wl_closure_destroy(struct wl_closure *closure)
{
	/* wl_closure_destroy has free() semantics */
	if (!closure)
		return;

	wl_closure_close_fds(closure);
	free(closure);
}
