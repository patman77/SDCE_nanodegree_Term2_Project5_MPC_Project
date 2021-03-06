#include "MPC.h"
#include <cppad/cppad.hpp>
#include <cppad/ipopt/solve.hpp>
#include <iostream>
#include <string>
#include <vector>
#include "Eigen-3.3/Eigen/Core"

using CppAD::AD;
using Eigen::VectorXd;

/**
 * DONE: Set the timestep length and duration
 */
//size_t N = 9;      // according to lesson 06. Putting It All Together
//double dt = 0.025; // start with 40 ms
size_t N = 10;       // according to video walkthrough
double dt = 0.1;     // according to video walkthrough, not too small. Totally 1 second into the future

// This value assumes the model presented in the classroom is used.
//
// It was obtained by measuring the radius formed by running the vehicle in the
//   simulator around in a circle with a constant steering angle and velocity on
//   a flat terrain.
//
// Lf was tuned until the the radius formed by the simulating the model
//   presented in the classroom matched the previous radius.
//
// This is the length from front to CoG that has a similar radius.
const double Lf = 2.67;

double ref_cte  = 0.0;
double ref_epsi = 0.0;
//const double mph2ms = 0.44704;
//double ref_v    = 40.0 * mph2ms; // in mph, convert to m/s
double ref_v    = 50.0;

// Taken from the MPC quiz:
// The solver takes all the state variables and actuator
// variables in a singular vector. Thus, we should to establish
// when one variable starts and another ends to make our lifes easier.
size_t x_start     = 0;
size_t y_start     = x_start + N;
size_t psi_start   = y_start + N;
size_t v_start     = psi_start + N;
size_t cte_start   = v_start + N;
size_t epsi_start  = cte_start + N;
size_t delta_start = epsi_start + N;
size_t a_start     = delta_start + N - 1;

class FG_eval {
 public:
  // Fitted polynomial coefficients
  VectorXd coeffs;
  FG_eval(VectorXd coeffs) { this->coeffs = coeffs; }

  typedef CPPAD_TESTVECTOR(AD<double>) ADvector;
  void operator()(ADvector& fg, const ADvector& vars) {
    /**
     * DONE: implement MPC
     * `fg` is a vector of the cost constraints, `vars` is a vector of variable 
     *   values (state & actuators)
     * NOTE: You'll probably go back and forth between this function and
     *   the Solver function below.
     */

    fg[0] = 0; // start with zero cost
#define USE_MPC_QUIZ_INSTEAD_OF_VIDEO_WALKTHROUGH
#undef USE_MPC_QUIZ_INSTEAD_OF_VIDEO_WALKTHROUGH
    // Reference State Cost
    /**
     * DONE: Define the cost related the reference state and
     *   anything you think may be beneficial.
     */
    // part of the cost based on reference state
    for(int t=0; t<N; ++t)
    {
#ifdef USE_MPC_QUIZ_INSTEAD_OF_VIDEO_WALKTHROUGH
      fg[0] += CppAD::pow(vars[cte_start + t], 2);       // minimize Cross Track Error for every time step
      fg[0] += CppAD::pow(vars[epsi_start + t], 2);      // minimize orientation error for every time step
      fg[0] += CppAD::pow(vars[v_start + t] - ref_v, 2); // minimize deviation to reference speed
#else // video walkthrough, different weighting
      fg[0] += 2000.0*CppAD::pow(vars[cte_start + t], 2);       // minimize Cross Track Error for every time step
      fg[0] += 2000.0*CppAD::pow(vars[epsi_start + t], 2);      // minimize orientation error for every time step
      fg[0] += 0.5   *CppAD::pow(vars[v_start + t] - ref_v, 2); // minimize deviation to reference speed
#endif
    }
    // minimize use of actuators
    for(int t=0; t<N-1; ++t)
    {
#ifdef USE_MPC_QUIZ_INSTEAD_OF_VIDEO_WALKTHROUGH
      fg[0] += CppAD::pow(vars[delta_start + t], 2); // minimize use of steering
      fg[0] += CppAD::pow(vars[a_start + t], 2);     // minimize use of acceleration
#else // video walkthrough, different weighting
      fg[0] += 50.0*CppAD::pow(vars[delta_start + t], 2); // minimize use of steering
      fg[0] += 5.0*CppAD::pow(vars[a_start + t], 2);     // minimize use of acceleration
#endif
    }
    // minimize value gap between sequential actuations
    for(int t=0; t<N-2; ++t)
    {
#ifdef USE_MPC_QUIZ_INSTEAD_OF_VIDEO_WALKTHROUGH
      fg[0] += CppAD::pow(vars[delta_start + t + 1] - vars[delta_start + t], 2); // minimize sequential steering gaps
      fg[0] += CppAD::pow(vars[a_start + t + 1] - vars[a_start + t], 2);         // minimize sequential acceleration gaps
#else // video walkthrough, different weighting
      fg[0] += 6000.0*CppAD::pow(vars[delta_start + t + 1] - vars[delta_start + t], 2); // minimize sequential steering gaps
      fg[0] += 10.0 *CppAD::pow(vars[a_start + t + 1] - vars[a_start + t], 2);         // minimize sequential acceleration gaps
#endif
    }

    //
    // Setup Constraints
    //
    // NOTE: In this section you'll setup the model constraints.

    // Initial constraints
    //
    // We add 1 to each of the starting indices due to cost being located at
    // index 0 of `fg`.
    // This bumps up the position of all the other values.
    fg[1 + x_start]    = vars[x_start];
    fg[1 + y_start]    = vars[y_start];
    fg[1 + psi_start]  = vars[psi_start];
    fg[1 + v_start]    = vars[v_start];
    fg[1 + cte_start]  = vars[cte_start];
    fg[1 + epsi_start] = vars[epsi_start];

    // The rest of the constraints
    for (int t = 1; t < N; ++t) {
      /**
       * DONE: Grab the rest of the states at t+1 and t.
       *   We have given you parts of these states below.
       */
      AD<double> x1 = vars[x_start + t];

      AD<double> x0 = vars[x_start + t - 1];
      AD<double> psi0 = vars[psi_start + t - 1];
      AD<double> v0 = vars[v_start + t - 1];

      // Here's `x` to get you started.
      // The idea here is to constraint this value to be 0.
      //
      // NOTE: The use of `AD<double>` and use of `CppAD`!
      // CppAD can compute derivatives and pass these to the solver.
      AD<double> y0 = vars[y_start + t - 1];
      AD<double> y1 = vars[y_start + t];
      AD<double> psi1 = vars[psi_start + t];
      AD<double> v1 = vars[v_start + t];
      AD<double> delta0 = vars[delta_start + t - 1];
      AD<double> a0 = vars[a_start + t - 1];
      AD<double> cte1 = vars[cte_start + t];
      AD<double> cte0 = vars[cte_start + t - 1];
      AD<double> epsi1 = vars[epsi_start + t];
      AD<double> epsi0 = vars[epsi_start + t - 1];

      // AD<double> f0 = coeffs[0] + coeffs[1] * x0; // wrong, this is for 1st order polynomial
      AD<double> f0 = coeffs[0] + coeffs[1] * x0 + coeffs[2] * x0 * x0 + coeffs[3] * x0 * x0 * x0; // 3rd order polynomial
      //AD<double> psides0 = CppAD::atan(coeffs[1]); // angle is between -PI/2 and PI/2, so atan is enough. variant for polynomial order 1
      AD<double> psides0 = CppAD::atan(3*coeffs[3]*x0*x0+2*coeffs[2]*x0+coeffs[1]); // angle is between -PI/2 and PI/2, so atan is enough. variant for polynomial order 3
      /**
       * DONE: Setup the rest of the model constraints
       */
      // Recall the equations for the model:
      // x_[t] = x[t-1] + v[t-1] * cos(psi[t-1]) * dt
      // y_[t] = y[t-1] + v[t-1] * sin(psi[t-1]) * dt
      // psi_[t] = psi[t-1] + v[t-1] / Lf * delta[t-1] * dt
      // v_[t] = v[t-1] + a[t-1] * dt
      // cte[t] = f(x[t-1]) - y[t-1] + v[t-1] * sin(epsi[t-1]) * dt
      // epsi[t] = psi[t] - psides[t-1] + v[t-1] * delta[t-1] / Lf * dt

      fg[1 + x_start + t   ] = x1 - (x0 + v0 * CppAD::cos(psi0) * dt);
      fg[1 + y_start + t   ] = y1 - (y0 + v0 * CppAD::sin(psi0) * dt);
      fg[1 + psi_start + t ] = psi1 - (psi0 + v0/Lf * delta0 * dt);
      fg[1 + v_start + t   ] = v1 - (v0 + a0 * dt);
      fg[1 + cte_start + t ] = cte1 - (f0 - y0 + (v0 * CppAD::sin(epsi0) * dt));
      fg[1 + epsi_start + t] = epsi1 - (psi0 - psides0 + (v0/Lf * delta0 * dt));
    }
  }
};

//
// MPC class definition implementation.
//
MPC::MPC() {}
MPC::~MPC() {}

std::vector<double> MPC::Solve(const VectorXd &state, const VectorXd &coeffs) {
  bool ok = true;
  typedef CPPAD_TESTVECTOR(double) Dvector;

  double x    = state[0];
  double y    = state[1];
  double psi  = state[2];
  double v    = state[3];
  double cte  = state[4];
  double epsi = state[5];

  /**
   * DONE: Set the number of model variables (includes both states and inputs).
   * For example: If the state is a 4 element vector, the actuators is a 2
   *   element vector and there are 10 timesteps. The number of variables is:
   *   4 * 10 + 2 * 9
   * in general: N timesteps means N-1 actuations
   * state is a 6 element vector: x, y, psi, v, cte, epsi
   * actuator is a 2 element vector: delta and a
   * so: 6*N+2*(N-1)
   */
  size_t n_vars = 6*N + 2*(N-1);
  /**
   * DONE: Set the number of constraints
   */
  size_t n_constraints = 6*N;

  // Initial value of the independent variables.
  // SHOULD BE 0 besides initial state.
  Dvector vars(n_vars);
  for (int i = 0; i < n_vars; ++i) {
    vars[i] = 0;
  }

  Dvector vars_lowerbound(n_vars);
  Dvector vars_upperbound(n_vars);
  /**
   * DONE: Set lower and upper limits for variables.
   */
  // Set all non-actuators upper and lowerlimits
  // to the max negative and positive values.
  for (int i = 0; i < delta_start; ++i) {
    vars_lowerbound[i] = -1.0e19;
    vars_upperbound[i] =  1.0e19;
  }


  // The upper and lower limits of delta are set to -25 and 25
  // degrees (values in radians).
  // NOTE: Feel free to change this to something else.
  for (int i = delta_start; i < a_start; ++i) {
#ifdef USE_MPC_QUIZ_INSTEAD_OF_VIDEO_WALKTHROUGH
    vars_lowerbound[i] = -0.436332;
    vars_upperbound[i] =  0.436332;
#else
    vars_lowerbound[i] = -0.436332*Lf;
    vars_upperbound[i] =  0.436332*Lf;
#endif
  }

  // Acceleration/decceleration upper and lower limits.
  // NOTE: Feel free to change this to something else.
  for (int i = a_start; i < n_vars; ++i) {
    vars_lowerbound[i] = -1.0;
    vars_upperbound[i] =  1.0;
  }
#ifdef LATENCY_HANDLING
  // new, to handle latency
  vars_lowerbound[delta_start]=prevDelta;
  vars_upperbound[delta_start]=prevDelta;
  vars_lowerbound[a_start]=prevA;
  vars_upperbound[a_start]=prevA;
#endif
  
  // Lower and upper limits for the constraints
  // Should be 0 besides initial state.
  Dvector constraints_lowerbound(n_constraints);
  Dvector constraints_upperbound(n_constraints);
  for (int i = 0; i < n_constraints; ++i) {
    constraints_lowerbound[i] = 0;
    constraints_upperbound[i] = 0;
  }
  constraints_lowerbound[x_start]    = x;
  constraints_lowerbound[y_start]    = y;
  constraints_lowerbound[psi_start]  = psi;
  constraints_lowerbound[v_start]    = v;
  constraints_lowerbound[cte_start]  = cte;
  constraints_lowerbound[epsi_start] = epsi;

  constraints_upperbound[x_start]    = x;
  constraints_upperbound[y_start]    = y;
  constraints_upperbound[psi_start]  = psi;
  constraints_upperbound[v_start]    = v;
  constraints_upperbound[cte_start]  = cte;
  constraints_upperbound[epsi_start] = epsi;

  // object that computes objective and constraints
  FG_eval fg_eval(coeffs);

  // NOTE: You don't have to worry about these options
  // options for IPOPT solver
  std::string options;
  // Uncomment this if you'd like more print information
  options += "Integer print_level  0\n";
  // NOTE: Setting sparse to true allows the solver to take advantage
  //   of sparse routines, this makes the computation MUCH FASTER. If you can
  //   uncomment 1 of these and see if it makes a difference or not but if you
  //   uncomment both the computation time should go up in orders of magnitude.
  options += "Sparse  true        forward\n";
  options += "Sparse  true        reverse\n";
  // NOTE: Currently the solver has a maximum time limit of 0.5 seconds.
  // Change this as you see fit.
  options += "Numeric max_cpu_time          0.5\n";

  // place to return solution
  CppAD::ipopt::solve_result<Dvector> solution;

  // solve the problem
  CppAD::ipopt::solve<Dvector, FG_eval>(
      options, vars, vars_lowerbound, vars_upperbound, constraints_lowerbound,
      constraints_upperbound, fg_eval, solution);

  // Check some of the solution values
  ok &= solution.status == CppAD::ipopt::solve_result<Dvector>::success;

  // Cost
  auto cost = solution.obj_value;
  std::cout << "Cost " << cost << std::endl;

  /**
   * DONE: Return the first actuator values. The variables can be accessed with
   *   `solution.x[i]`.
   *
   * {...} is shorthand for creating a vector, so auto x1 = {1.0,2.0}
   *   creates a 2 element double vector.
   */

  std::vector<double> result;

#ifndef LATENCY_HANDLING
  result.push_back(solution.x[delta_start]); // without latency handling
  result.push_back(solution.x[a_start]);     // without latency handling
#else
  result.push_back(solution.x[delta_start+1]); // new with latency handling
  result.push_back(solution.x[a_start+1]);     // new with latency handling
  prevDelta = solution.x[delta_start+1];
  prevA     = solution.x[a_start+1];
#endif

  for(int i=0; i<N-1; ++i)
  {
    result.push_back(solution.x[x_start + i + 1]);
    result.push_back(solution.x[y_start + i + 1]);
  }
  return result;
}
