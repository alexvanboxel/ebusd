/*
 * Copyright (C) Roland Jax 2012-2014 <ebusd@liwest.at>
 *
 * This file is part of ebusd.
 *
 * ebusd is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * ebusd is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with ebusd. If not, see http://www.gnu.org/licenses/.
 */

#ifndef LIBUTILS_NOTIFY_H_
#define LIBUTILS_NOTIFY_H_

#include <unistd.h>
#include <fcntl.h>

/**
 * @brief class to notify other thread per pipe.
 */
class Notify
{

public:
	/**
	 * @brief constructs a new instance and do notifying.
	 */
	Notify();

	/**
	 * @brief destructor.
	 */
	~Notify();

	/**
	 * @brief file descriptor to watch for notify event.
	 * @return the notification value.
	 */
	int notifyFD() { return m_recvfd; }

	/**
	 * @brief write notify event to file descriptor.
	 * @return result of writing notification.
	 */
	int notify() const { return write(m_sendfd,"1",1); }

private:
	/** file descriptor to watch */
	int m_recvfd;

	/** file descriptor to notify */
	int m_sendfd;

};

#endif // LIBUTILS_NOTIFY_H_


