# HPCsim

HPCsim is lightweight framework for Monte Carlo simulations in the HPC field.

It consists in a single executable which contains the simulation core, capable of spawning as many threads as the user wants. The simulation itself is out of HPCsim and consists in a shared object library that HPCsim will load at run time and execute.

The user only has to focus on its modeling and not on the simulation itself.

The simulation core, as currently written, serves three major goals: efficiency, numerical reproducibility, and statistically sound results. That way, each event has its own pseudo-random stream and is identified with the hash of the initial state of such stream. The user doesn't need to know where, how their events where executed. Only the ID matters to uniquely identify the events.

Because of its design, this simulation core is well suited for simulations where the simulated objects are independant, such as high energy physics Monte Carlo simulations.

Because it has virtually no dependencies, this simulation framework can be ported over Xeon Phi (k1om) without troubles.

# Example

So far, HPCsim comes with a single example, used to compute Pi. To use it, just build the whole repository (that's the default) and then simply run: ./HPCsim/HPCsim -s examples/Pi/libPi.so

By default, it will compute 1,000,000 points to be able to approximate Pi value. All these events will be spread (still by default) on 100 events (each event computing 10,000 points) using a single thread. The results will be outputed in a binary file, called, by default HPCsim.out.

You can adjust the number of events, of threads, and the starting events by using HPCsim parameters:

Usage: ./HPCsim/HPCsim --simulation|-s name.so [--threads|-t X --first|-f X --events|-e X --output|-o name]

	- Simulation: path of the shared library containing the simulation
	
	- Threads: amount of threads to use for computing (min 1). Beware an extra thread will be used for results writing
	
	- First: start the event loop at this event
	
	- Events: number of events to compute
	
	- Output: name of the output file to write

To really compute the value of Pi, given all these random points, just use the "ResPi" application, that will by default read the HPCsim.out file. It will output the approximated Pi value.

You'll notice that given the same amount of events, whatever the number of threads you'll spawn, you'll get the exact same result.

# Writing a simulation

In order to write a simulation using HPCsim, all you need to is to implement a shared object (as shown with Pi simulation) that implements a few functions that HPCsim will call.

There are a few things to know about these functions regarding parallelism. SimulationInit() is purely sequential and called only once. Same goes for RunInit(), RunClear(), SimulationUnload(). EventInit() can be called while other events are being proceed, BUT there's only one call to EventInit() at a time. It is the right place to initialize critical parallel stuff for the event, same goes to EventClear(). EventRun() is the hot loop of your simulation. That one is always called after an EventInit() and before an EventClear(). It is purely parallel, and you cannot make any assumption about the state of shared data you'd used in it.

In case you built your simulation with -DUSE_PILOT_THREAD=1, then, you have to implement PilotInit() and PilotClear(). These work on the same model than EventInit() and EventClear(). A pilot will run several events in the same thread, sequential, so you may want to share a context between all these.

In order to allow the user to perform Monte Carlo simulation, two functions are exported to the user: RandU01() and QueueResult(). The first one is returning an uniformly distributed between 0 and 1 pseudo-random number. The stream it comes from is local to the event and independant from the streams of the others events. This mandatory to have sound statistical results. QueueResult() is there to allow you to write in an async way your results. You have to match the TResult structure for writing your resuls. You don't have to fill in fId field, HPCsim will do it for you. You only need to set how much (in bytes) you consume in the fResult buffer. Only these bytes will be written to disk.

As a reminder, for performances reasons, during the simulation, it is highly recommanded NOT TO perform any IO, be it to console or to disk. If you want to write to the disk, use the QueueResult() function that uses a background writer thread in order not to impact on computation performances. Also, any read you should do, do it during init, and share it to your events (if RO) or copy it to your events (if RW)

# Acknowledgements

David R.C. Hill for his PhD supervision, and his article: 
Hill R.C., D. (2015), "Parallel Random Numbers, Science and reproducibility", in IEEE Computing in Science and Engineering, 17(4) pp: 66-71.

Pierre l'Ecuyer for his packaging of MRG2k3a as RngStream which is the heart of the HPCsim framework.

Aaron Gifford for his lightweight SHA hash library which was the missing piece of easy porting to Xeon Phi. It's been slightly modified locally and patch sent upstream.
