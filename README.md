ibverbs-gptl-wrapper
====================

Wrapper for ibverbs to do automatic instrumentation using GPTL performance library



This is a wrapper to ibverbs library.

This is helpful in automatic instrumentation fo ibverbs library using GPTL.

Since ibverbs itself is an API for a vendors' implementation to RDMA verbs,
this would help one understand the function level overhead of implementing those verbs.

Note that since the data path calls - ibv_post_send / ibv_post_recv / ibv_poll_cq ... etc are "inlined"
there is no way to wrap them.


For this, one need to write a wrapper to the function that implements these data path calls.
* At this point, the wrapper implements only for mlx4 and ocrdma.
* If you have a specific interest in a specific vendor's implementation, send me an email to ramnath.sagar@gmail.com

Note: In some scenario, this might not work, if the libraries are build without debugging symbols, which is inherent for this technique to work.
In this case one needs to manually compile the libraries with debugging symbols.

