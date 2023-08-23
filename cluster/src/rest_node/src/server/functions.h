#ifndef REST_NODE_SRC_FUNCTIONS
#define REST_NODE_SRC_FUNCTIONS

#include <mpi/mpi.h>
#include "../../../common.h"

namespace uh::rest
{

//------------------------------------------------------------------------------

    template<bool isRequest>
    void putObject(const s3_parser<isRequest>& s3_parser, const uh::cluster::cluster_ranks& m_cluster_plan)
    {
        auto rc = MPI_Send(s3_parser.m_parsed_struct.body_stream.str().data(), s3_parser.m_parsed_struct.body_stream.str().size(), MPI_CHAR,
                           m_cluster_plan.dedupe_ranks.front(), uh::cluster::message_types::DEDUPE_REQ, MPI_COMM_WORLD);

        std::cout << s3_parser.m_parsed_struct.body_stream.str().data();

        // send message before abort to the client
        if (rc != MPI_SUCCESS) [[unlikely]]
        {
            MPI_Abort(MPI_COMM_WORLD, rc);
        }

        /* receiving a message */
        MPI_Status status;
        int inc_message_size;

        rc = MPI_Probe(MPI_ANY_SOURCE, MPI_ANY_TAG, MPI_COMM_WORLD, &status);
        if (rc != MPI_SUCCESS) [[unlikely]]
        {
            MPI_Abort(MPI_COMM_WORLD, rc);
        }

        MPI_Get_count(&status, MPI_CHAR, &inc_message_size);

        uh::cluster::address addr (static_cast<size_t>(inc_message_size)/sizeof(uh::cluster::wide_span));

        rc = MPI_Recv(addr.data(), inc_message_size, MPI_CHAR, m_cluster_plan.dedupe_ranks.front(), uh::cluster::message_types::DEDUPE_RESP,
                      MPI_COMM_WORLD, &status);
        if (rc != MPI_SUCCESS) [[unlikely]]
        {
            MPI_Abort(MPI_COMM_WORLD, rc);
        }
        const auto effective_size = addr.back().size;

        std::cout << "Effective size is: " << effective_size << std::endl;

//                // send to phonebook
//                rc = MPI_Send(addr.data(), inc_message_size-sizeof(uh::cluster::wide_span), MPI_CHAR,
//                                   m_cluster_plan.dedupe_ranks.front(), uh::cluster::message_types::, MPI_COMM_WORLD);
//
//                if (rc != MPI_SUCCESS) [[unlikely]]
//                {
//                    MPI_Abort(MPI_COMM_WORLD, rc);
//                }
    }

//------------------------------------------------------------------------------

    template<bool isRequest>
    void getObject(const s3_parser<isRequest>& s3_parser, const uh::cluster::cluster_ranks& m_cluster_plan)
    {

    }

} // namespace uh::rest

#endif // REST_NODE_SRC_FUNCTIONS