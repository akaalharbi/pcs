#ifndef MITM_MPI_SENDER
#define MITM_MPI_SENDER
#include <err.h>
#include <vector>

#include <mpi.h>

#include "common.hpp"
#include "engine_common.hpp"
#include "mpi/common.hpp"

namespace mitm {

template<class ConcreteProblem>
void sender(ConcreteProblem& Pb, const MpiParameters &params)
{
	for (;;) {
		/* get data from controller */
		u64 msg[3];   // i, root_seed, stop?
		MPI_Bcast(msg, 3, MPI_UINT64_T, 0, params.world_comm);
		if (msg[2] != 0)
			return;      // controller tells us to stop

		u64 i = msg[0];
    	u64 n_dp = 0;    // #DP found since last report
    	Pb.n_eval = 0;
		SendBuffers sendbuf(params.inter_comm, TAG_POINTS, 3 * params.buffer_capacity);
    	double last_ping = wtime();
		for (u64 j = msg[1] + 3*params.local_rank;; j += 3*params.n_send) {   // add an odd number to avoid problems mod 2^n...

			/* call home? */
            if ((n_dp % 10000 == 9999) && (wtime() - last_ping >= params.ping_delay)) {
				last_ping = wtime();
            	MPI_Send(&n_dp, 1, MPI_UINT64_T, 0, TAG_SENDER_CALLHOME, params.world_comm);
				n_dp = 0;

            	int assignment;
            	MPI_Recv(&assignment, 1, MPI_INT, 0, TAG_ASSIGNMENT, params.world_comm, MPI_STATUS_IGNORE);
            	if (assignment == NEW_VERSION) {        /* new broadcast */
            	   	sendbuf.flush();   
            		break;
            	}
            }

			/* start a new chain from a fresh "random" starting point */
			u64 start = j & Pb.mask;
            auto dp = generate_dist_point(Pb, i, params, start);
            if (not dp) {
                // ctr.dp_failure();
                continue;       /* bad chain start */
            }

			n_dp += 1;
            auto [end, len] = *dp;

            u64 hash = (end * 0xdeadbeef) % 0x7fffffff;
            int target_recv = ((int) hash) % params.n_recv;
            sendbuf.push3(start, end, len, target_recv);
		}

		// now is a good time to collect stats
		//             #f send,   #f recv,    collisions, bytes sent
		u64 imin[4] = {Pb.n_eval, ULLONG_MAX, ULLONG_MAX, sendbuf.bytes_sent};
		u64 imax[4] = {Pb.n_eval, 0,          0,          sendbuf.bytes_sent};
		u64 iavg[4] = {Pb.n_eval, 0,          0,          sendbuf.bytes_sent};
		MPI_Reduce(imin, NULL, 4, MPI_UINT64_T, MPI_MIN, 0, params.world_comm);
		MPI_Reduce(imax, NULL, 4, MPI_UINT64_T, MPI_MAX, 0, params.world_comm);
		MPI_Reduce(iavg, NULL, 4, MPI_UINT64_T, MPI_SUM, 0, params.world_comm);
		//                send wait             recv wait
		double dmin[2] = {sendbuf.waiting_time, HUGE_VAL};
		double dmax[2] = {sendbuf.waiting_time, 0};
		double davg[2] = {sendbuf.waiting_time, 0};
		MPI_Reduce(dmin, NULL, 2, MPI_DOUBLE, MPI_MIN, 0, params.world_comm);
		MPI_Reduce(dmax, NULL, 2, MPI_DOUBLE, MPI_MAX, 0, params.world_comm);
		MPI_Reduce(davg, NULL, 2, MPI_DOUBLE, MPI_SUM, 0, params.world_comm);
	}
}

}
#endif