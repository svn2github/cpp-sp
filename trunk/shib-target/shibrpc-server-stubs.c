/*
 * This is sample code generated by rpcgen.
 * These are only templates and you can use them
 * as a guideline for developing your own functions.
 */

#include "shibrpc.h"

bool_t
shibrpc_ping_2_svc(int *argp, int *result, struct svc_req *rqstp)
{
	bool_t retval;

	/*
	 * insert server code here
	 */

	return retval;
}

bool_t
shibrpc_new_session_2_svc(shibrpc_new_session_args_2 *argp, shibrpc_new_session_ret_2 *result, struct svc_req *rqstp)
{
	bool_t retval;

	/*
	 * insert server code here
	 */

	return retval;
}

bool_t
shibrpc_get_session_2_svc(shibrpc_get_session_args_2 *argp, shibrpc_get_session_ret_2 *result, struct svc_req *rqstp)
{
	bool_t retval;

	/*
	 * insert server code here
	 */

	return retval;
}

bool_t
shibrpc_statemgr_2_svc(shibrpc_statemgr_args_2 *argp, shibrpc_statemgr_ret_2 *result, struct svc_req *rqstp)
{
	bool_t retval;

	/*
	 * insert server code here
	 */

	return retval;
}

int
shibrpc_prog_2_freeresult (SVCXPRT *transp, xdrproc_t xdr_result, caddr_t result)
{
	xdr_free (xdr_result, result);

	/*
	 * Insert additional freeing code here, if needed
	 */

	return 1;
}
