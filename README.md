![Figurer: Sometimes you need a little help with your figuring](https://github.com/ericlavigne/figurer/raw/master/images/figurer.png)

Figurer helps with messy, real world, planning problems. It can help
you drive a car or fly a quadcopter by choosing the best sequence
of actions. But figurer can't do these things for you.

You will still need to thoroughly define the problem and suggest possible
solutions. Then Figurer uses your predictions about cause-and-effect, your
suggestions for likely next moves, and your judgement about the value of
each possible result. Figurer tries a variety of possibilities,
looks ahead 10 steps, and finds the right combination of your suggestions
that will best satisfy your goals.

Figurer was designed for use with self-driving cars, yet is applicable
to any planning problem for which the physics and desired outcome are
well understood.

## Installation

[![Clojars Project](https://img.shields.io/clojars/v/figurer.svg)](https://clojars.org/figurer)

Add the following dependency to your `project.clj` file:

    [figurer "0.1.0"]

## Usage

More documentation coming soon. For now, please review
[CarND-MPC-Clojure](https://github.com/ericlavigne/CarND-MPC-Clojure)
for an example of driving a car in a simulator.

## License

Copyright © 2018 Eric Lavigne

Distributed under the Eclipse Public License either version 1.0 or (at
your option) any later version.
