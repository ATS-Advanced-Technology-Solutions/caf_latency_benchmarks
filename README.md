# What is this project about

This project provides a suite of benchmarks to test **[C++ Actor Framework](https://www.actor-framework.org/)**  performances with a specific attention to _latencies_.


# How this benchmark works

In this benchmark actors send messages to each other with configurable payloads in order to measure performances of CAF framework.

### Executor actor

- Build (eventually multiple) pipeline actors chain(s) by calling topology builder.
- Starts the benchmark sending messages to the head of chain(s) or at the rate generator.
- Sends results and statistics to the `result_collector_actor`.

### Rate generator

- Sends messages at a given rate to the head of the pipeline.
- If there are multiple chains, it sends messages with the specified policy to the heads.

### Topology builder

- Spawns the requested number of benchmark actors (one of them may be detached).
- Returns the head of the chain to the `executor_actor`.

### Benchmark actor

- A `benchmark_actor` knows its previous and its next pipeline item and sends messages forth and back to them (depending on benchmark type).
- For latency tracking type benchmark, the last actor of the pipeline take the _stop time_ in order to compute the time between the first message dispatch and the stop time.
- For _cross-times_ tracking type benchmark, each benchmark actor adds an hop to the _cross-times_ istance in the message.

### Result collector actor

- Collects statistics on system resources usage.
- Writes statistics collected from the `executor_actor` to file.


# How to use this benchmark

In order to run the executable needs the `caf_latency_benchmarks.ini` file in a folder named config.

It will output results in a semicolon-separated test file configurable with the `result_path`, `extension` and `output_file`  parameters of _.ini_ file.

Run executable with `--help` to see available command line options.

See the provided _.ini_ file to see al available parameters.

### Message passing overhead (parallel benchmark only)

`-P, --benchmark.payload_type = [NONE|COMPLEX]`

CAF Benchmark support two message type:

- `NONE`: a weightless messages (pure latency measure)
- `COMPLEX`: message composed by a _std::string_ and a _C struct_ with some fixed data and a _std::vector&lt;char&gt;_

The string lenght and vector size configuration parameters:

`--payload.string_length, --payload.vector_size`

### Actor topology

#### Number of actors

`-N, --pipeline_actors`

Linear topology with a fixed number of actors.

#### Detached actor

` --benchmark.detach = [0 |...| N-1]`

An actor in the pipeline can be spawned as detached.

#### Chains number (parallel benchmark type only)

`--parallel.chains_number`

### Benchmark type

`-t, –benchmark.benchmark_type = [SERIAL|PARALLEL]`

- `SERIAL`: Sends message to head when the previous one arrived ad the end of the chain
- `PARALLEL`: Sends messages to head at a given rate

Parallel message rate can be specified with

`-r, --parallel.rate`

#### forth\_and\_back

`--benchmark.forth_and_back = [0|1]`

Specifies if message must run from head to tail and then back to head again.

#### Sending order (multiple chain topology only)

`--parallel.sending_order = [0|1]`

Sending order policy to the head of the chains(s):

- `0`: Random
- `1`: Round-Robin

#### Tracking type (for complex messages only)

`-K, --benchmark.tracking_type = [LATENCY|XTIMES]`

- `LATENCY:` track latencies between pipeline head and tail only
- `XTIMES:` track idividual actor latencies (_cross-times_)

### Output

#### Filename specification

`--benchmark.result_path`

Folder path where to store output files.

`--benchmark.output_file`

File name where to store the results (__serial_ or __parallel_ will be added depending on the benchmark type).

` --benchmark.extension`

Extension of the output file.

Depending on the benchmark type (serial / parallel), the output filename will be constructed as:

`result_path + benchmark_type + output_file + extension`

#### Output Content

**Serial benchmark content**

- _number of actors_: Number of actors in the pipeline
- _net latency (us)_:  average time to dispatch a message from the head to the tail of the chain / number of actors
- _gross latency (us)_: Computed as `benchmark time / (number of sent messages * number of actors)`
- _message rate (msg/sec)_: Messages per second received by the `executor_actor`
- _cpu usage ave:_ Cpu usage average
- _ram usage ave (MB)_: Memory usage average


##### **Parallel benchmark content**

- _number of actors_: Number of actors in the pipeline
- _sender message rate (msg/s):_ Message rate generated by `rate_generator`
- _percentage sender error_: Percentage error between requested and generated rate
- _actor latency (us)_: average time to dispatch a message from the head to the tail of the chain / number of actors
- _receiver message rate (msg/s)_: Message rate received by `executor_actor`
- _percentage receiver error_: Percentage error between requested and received rate
- _cpu usage ave_: Cpu usage average
- _ram usage ave (MB)_: Memory usage average

For _cross-times_ tracking type, three additional field will be output:

- _detached\_send:_ average latency needed to send a message from a detached actor
- _detached\_receive_: average latency needed to receive a message from a detached actor
- _not\_detached_: average latency needed to send or receive a message between non detached actors.
