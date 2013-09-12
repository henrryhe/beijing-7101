/*
 * connection.c
 *
 * Copyright (C) STMicroelectronics Limited 2002. All rights reserved.
 *
 * Check the transport returns the correct errors when interacting with
 * the port namespace.
 *
 * Further check the remote port invalidation sequencing.
 */

#include "harness.h"

static EMBX_TRANSPORT transport;
static EMBX_PORT inPort, outPort, testPort;

int main()
{
	EMBX_VOID *buffer;
	EMBX_RECEIVE_EVENT event;

	harness_boot();
	transport = harness_openTransport();

	/* try connecting to a port that does not exist */
	EMBX_E(EMBX_PORT_NOT_BIND, Connect(transport, "not-a-port", &inPort));

	/* now try creating a port with the same name */
	EMBX(CreatePort(transport, "test" LOCAL, &testPort));
	EMBX_E(EMBX_ALREADY_BIND, CreatePort(transport, "test" LOCAL, &inPort));

	/* now try closing that port and trying again */
	EMBX(ClosePort(testPort));
	EMBX(CreatePort(transport, "test" LOCAL, &testPort));
	EMBX_E(EMBX_ALREADY_BIND, CreatePort(transport, "test" LOCAL, &inPort));

	/* now clean up by closing the port */
	EMBX(ClosePort(testPort));

	/* exchange ports (rendezvous point here) */
	EMBX(CreatePort  (transport, "connection" LOCAL,  &inPort ));
	EMBX(ConnectBlock(transport, "connection" REMOTE, &outPort));

	/* open an port already bound on the remote CPU */
	EMBX_E(EMBX_ALREADY_BIND, CreatePort(transport, "connection" REMOTE, &testPort));

	/* check we can open a second remote port (using the Connect API since
	 * we know the port already exists)
	 */
	EMBX(Connect(transport, "connection" REMOTE, &testPort));
	EMBX(ClosePort(testPort));

	/* exchange messages (rendezvous point here) */
	EMBX(Alloc(transport, 4, &buffer));
	EMBX(SendMessage(outPort, buffer, 0));
	EMBX(ReceiveBlock(inPort, &event));
	EMBX(Free(event.data));

	/* close the masters local port (will invalidate remote ports) */
	MASTER(EMBX(ClosePort(inPort)));

	/* let the slave know we have closed our port */
	MASTER(EMBX(Alloc(transport, 4, &buffer)));
	MASTER(EMBX(SendMessage(outPort, buffer, 0)));
	SLAVE (EMBX(ReceiveBlock(inPort, &event)));
	SLAVE (EMBX(Free(event.data)));

	/* now use the remote port on the slave to check it really has
	 * been invalidated
	 */
	SLAVE (EMBX(Alloc(transport, 4, &buffer)));
	SLAVE (EMBX_E(EMBX_INVALID_PORT, SendMessage(outPort, buffer, 0)));
	SLAVE (EMBX(Free(buffer)));

	/* now re-establish to connection */
	SLAVE (EMBX(ClosePort(outPort)));
	MASTER(EMBX(CreatePort  (transport, "connection" LOCAL,  &inPort )));
	SLAVE (EMBX(ConnectBlock(transport, "connection" REMOTE, &outPort)));

	/* now invalidate the slaves local port (will invalidate remote ports) */
	SLAVE (EMBX(InvalidatePort(inPort)));

	/* let the master know we have invalidated our port */
	SLAVE (EMBX(Alloc(transport, 4, &buffer)));
	SLAVE (EMBX(SendMessage(outPort, buffer, 0)));
	MASTER(EMBX(ReceiveBlock(inPort, &event)));
	MASTER(EMBX(Free(event.data)));

	/* now use the remote port on the master to check it really has 
	 * been invalidated
	 */
	MASTER(EMBX(Alloc(transport, 4, &buffer)));
	MASTER(EMBX_E(EMBX_INVALID_PORT, SendMessage(outPort, buffer, 0)));
	MASTER(EMBX(Free(buffer)));

	/* try to receive on the slave and check the error codes are
	 * correct
	 */
	SLAVE (EMBX_E(EMBX_INVALID_PORT, Receive(inPort, &event)));
	SLAVE (EMBX_E(EMBX_INVALID_PORT, ReceiveBlock(inPort, &event)));
	
	/* the final thing the slave does before everybody starts to clean
	 * up is notify the master that it is about to shutdown
	 */
	SLAVE (EMBX(Alloc(transport, 4, &buffer)));
	SLAVE (EMBX(SendMessage(outPort, buffer, 0)));
	MASTER(EMBX(ReceiveBlock(inPort, &event)));
	MASTER(EMBX(Free(event.data)));

	/* tidy up and close down */
	EMBX(ClosePort(inPort));
	EMBX(ClosePort(outPort));

	MASTER(harness_passed());
	SLAVE (harness_waitForShutdown());

	return 0;
}
