/*
 * Please do not edit this file.
 * It was generated using rpcgen.
 */

#ifndef _SHIBRPC_H_RPCGEN
#define _SHIBRPC_H_RPCGEN

#include <rpc/rpc.h>

#ifdef HAVE_PTHREAD
# include <pthread.h>
#endif

#ifdef __cplusplus
extern "C" {
#endif


struct shibrpc_new_session_args_2 {
	int supported_profiles;
	char *application_id;
	char *packet;
	char *recipient;
	char *client_addr;
};
typedef struct shibrpc_new_session_args_2 shibrpc_new_session_args_2;

struct shibrpc_new_session_ret_2 {
	char *status;
	char *target;
	char *cookie;
	char *provider_id;
};
typedef struct shibrpc_new_session_ret_2 shibrpc_new_session_ret_2;

struct shibrpc_get_session_args_2 {
	char *application_id;
	char *cookie;
	char *client_addr;
};
typedef struct shibrpc_get_session_args_2 shibrpc_get_session_args_2;

struct shibrpc_get_session_ret_2 {
	char *status;
	int profile;
	char *provider_id;
	char *auth_statement;
	char *attr_response_pre;
	char *attr_response_post;
};
typedef struct shibrpc_get_session_ret_2 shibrpc_get_session_ret_2;

struct shibrpc_end_session_args_2 {
	char *cookie;
};
typedef struct shibrpc_end_session_args_2 shibrpc_end_session_args_2;

struct shibrpc_end_session_ret_2 {
	char *status;
};
typedef struct shibrpc_end_session_ret_2 shibrpc_end_session_ret_2;

#define SHIBRPC_PROG 123456
#define SHIBRPC_VERS_2 2

#if defined(__STDC__) || defined(__cplusplus)
#define shibrpc_ping 0
extern  enum clnt_stat shibrpc_ping_2(int *, int *, CLIENT *);
extern  bool_t shibrpc_ping_2_svc(int *, int *, struct svc_req *);
#define shibrpc_new_session 1
extern  enum clnt_stat shibrpc_new_session_2(shibrpc_new_session_args_2 *, shibrpc_new_session_ret_2 *, CLIENT *);
extern  bool_t shibrpc_new_session_2_svc(shibrpc_new_session_args_2 *, shibrpc_new_session_ret_2 *, struct svc_req *);
#define shibrpc_get_session 2
extern  enum clnt_stat shibrpc_get_session_2(shibrpc_get_session_args_2 *, shibrpc_get_session_ret_2 *, CLIENT *);
extern  bool_t shibrpc_get_session_2_svc(shibrpc_get_session_args_2 *, shibrpc_get_session_ret_2 *, struct svc_req *);
#define shibrpc_end_session 3
extern  enum clnt_stat shibrpc_end_session_2(shibrpc_end_session_args_2 *, shibrpc_end_session_ret_2 *, CLIENT *);
extern  bool_t shibrpc_end_session_2_svc(shibrpc_end_session_args_2 *, shibrpc_end_session_ret_2 *, struct svc_req *);
extern int shibrpc_prog_2_freeresult (SVCXPRT *, xdrproc_t, caddr_t);

#else /* K&R C */
#define shibrpc_ping 0
extern  enum clnt_stat shibrpc_ping_2();
extern  bool_t shibrpc_ping_2_svc();
#define shibrpc_new_session 1
extern  enum clnt_stat shibrpc_new_session_2();
extern  bool_t shibrpc_new_session_2_svc();
#define shibrpc_get_session 2
extern  enum clnt_stat shibrpc_get_session_2();
extern  bool_t shibrpc_get_session_2_svc();
#define shibrpc_end_session 3
extern  enum clnt_stat shibrpc_end_session_2();
extern  bool_t shibrpc_end_session_2_svc();
extern int shibrpc_prog_2_freeresult ();
#endif /* K&R C */

/* the xdr functions */

#if defined(__STDC__) || defined(__cplusplus)
extern  bool_t xdr_shibrpc_new_session_args_2 (XDR *, shibrpc_new_session_args_2*);
extern  bool_t xdr_shibrpc_new_session_ret_2 (XDR *, shibrpc_new_session_ret_2*);
extern  bool_t xdr_shibrpc_get_session_args_2 (XDR *, shibrpc_get_session_args_2*);
extern  bool_t xdr_shibrpc_get_session_ret_2 (XDR *, shibrpc_get_session_ret_2*);
extern  bool_t xdr_shibrpc_end_session_args_2 (XDR *, shibrpc_end_session_args_2*);
extern  bool_t xdr_shibrpc_end_session_ret_2 (XDR *, shibrpc_end_session_ret_2*);

#else /* K&R C */
extern bool_t xdr_shibrpc_new_session_args_2 ();
extern bool_t xdr_shibrpc_new_session_ret_2 ();
extern bool_t xdr_shibrpc_get_session_args_2 ();
extern bool_t xdr_shibrpc_get_session_ret_2 ();
extern bool_t xdr_shibrpc_end_session_args_2 ();
extern bool_t xdr_shibrpc_end_session_ret_2 ();

#endif /* K&R C */

#ifdef __cplusplus
}
#endif

#endif /* !_SHIBRPC_H_RPCGEN */
