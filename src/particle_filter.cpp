/*
 * particle_filter.cpp
 *
 *  Created on: Dec 12, 2016
 *      Author: Tiffany Huang
 */

#include <random>
#include <algorithm>
#include <iostream>
#include <numeric>
#include <math.h> 
#include <iostream>
#include <sstream>
#include <string>
#include <iterator>

#include "particle_filter.h"

using namespace std;

static default_random_engine gen;
void ParticleFilter::init(double x, double y, double theta, double std[]) {
	// TODO: Set the number of particles. Initialize all particles to first position (based on estimates of 
	//   x, y, theta and their uncertainties from GPS) and all weights to 1. 
	// Add random Gaussian noise to each particle.
	// NOTE: Consult particle_filter.h for more information about this method (and others in this file).
	num_particles = 110;

	// This line creates a normal (Gaussian) distribution for x
	normal_distribution<double> dist_x(x, std[0]);
	normal_distribution<double> dist_y(y, std[1]);
	normal_distribution<double> dist_theta(theta, std[2]);
	
	//default_random_engine gen;

	for (int i = 0; i < num_particles; i++){
		Particle p; 
		p.id = i; 
		p.x = dist_x(gen); 
	 	p.y = dist_y(gen);
	 	p.theta = dist_theta(gen);
	 	p.weight = 1.0;

	 	particles.push_back(p);
	}
	
	is_initialized = true; //since it is present as a boolean in the header file 
}

void ParticleFilter::prediction(double delta_t, double std_pos[], double velocity, double yaw_rate) {
	// TODO: Add measurements to each particle and add random Gaussian noise.
	// NOTE: When adding noise you may find std::normal_distribution and std::default_random_engine useful.
	//  http://en.cppreference.com/w/cpp/numeric/random/normal_distribution
	//  http://www.cplusplus.com/reference/random/default_random_engine/
	//default_random_engine gen;

	// This line creates a normal (Gaussian) distribution for x
	normal_distribution<double> dist_x_2(0, std_pos[0]);
	
	// TODO: Create normal distributions for y and theta
	normal_distribution<double> dist_y_2(0, std_pos[1]);
	normal_distribution<double> dist_theta_2(0, std_pos[2]);

	for (int i = 0; i < num_particles; i++){

	 if (fabs(yaw_rate) < 0.00001) {  //since abs only works on integers, we need fabs for floating points
      		particles[i].x += velocity * delta_t * cos(particles[i].theta);// + dist_x_2(gen);
      		particles[i].y += velocity * delta_t * sin(particles[i].theta);// + dist_y_2(gen);
		//particles[i].theta += dist_theta_2(gen);
    	 }	 
    	 else {
      		particles[i].x += velocity / yaw_rate * (sin(particles[i].theta + yaw_rate * delta_t) - sin(particles[i].theta));// + dist_x_2(gen);
      		particles[i].y += velocity / yaw_rate * (cos(particles[i].theta) - cos(particles[i].theta + yaw_rate * delta_t)) ;//+ dist_y_2(gen);
      		particles[i].theta += yaw_rate * delta_t;
    	 }
	 
	 //smarter step : one time execution
	 particles[i].x += dist_x_2(gen);
	 particles[i].y += dist_y_2(gen);
	 particles[i].theta += dist_theta_2(gen);

  	}

}



void ParticleFilter::dataAssociation(std::vector<LandmarkObs> predicted, std::vector<LandmarkObs>& observations) {
	// TODO: Find the predicted measurement that is closest to each observed measurement and assign the 
	//   observed measurement to this particular landmark.
	// NOTE: this method will NOT be called by the grading code. But you will probably find it useful to 
	//   implement this method and use it as a helper during the updateWeights phase.
	
	for (int i = 0; i < observations.size(); i++){
	 LandmarkObs curr_obs = observations[i];
	 double min_dist = numeric_limits<double>::max(); // very high distance for starters
	 
	 int map_id = -99;

 	 for (int j = 0; j < predicted.size(); j++){
	  LandmarkObs pred_obs = predicted[j];

	  double curr_dist = dist(curr_obs.x, curr_obs.y, pred_obs.x, pred_obs.y);

	  if (curr_dist < min_dist){
		min_dist = curr_dist;
		map_id = pred_obs.id;
	  }
	 }
	 observations[i].id = map_id;  
	}


}


void ParticleFilter::updateWeights(double sensor_range, double std_landmark[], 
		const std::vector<LandmarkObs> &observations, const Map &map_landmarks) {
	// TODO: Update the weights of each particle using a mult-variate Gaussian distribution. You can read
	//   more about this distribution here: https://en.wikipedia.org/wiki/Multivariate_normal_distribution
	// NOTE: The observations are given in the VEHICLE'S coordinate system. Your particles are located
	//   according to the MAP'S coordinate system. You will need to transform between the two systems.
	//   Keep in mind that this transformation requires both rotation AND translation (but no scaling).
	//   The following is a good resource for the theory:
	//   https://www.willamette.edu/~gorr/classes/GeneralGraphics/Transforms/transforms2d.htm
	//   and the following is a good resource for the actual equation to implement (look at equation 
	//   3.33
	//   http://planning.cs.uiuc.edu/node99.html

	// the website does not work UIUC
	// Referred code syntax from: 
	// https://github.com/jeremy-shannon/CarND-Kidnapped-Vehicle-Project/blob/master/src/particle_filter.cpp
	
	for (int i = 0; i < num_particles; i++){
	 double par_x = particles[i].x;
	 double par_y = particles[i].y;
	 double par_theta = particles[i].theta;
	 
	 // Step 1: Filter map landmarks to keep only those which are in the sensor range of current particle
	 vector<LandmarkObs> predictions;
	 
 	 for (int j = 0; j < map_landmarks.landmark_list.size(); j++){
		float m_x = map_landmarks.landmark_list[j].x_f;
		float m_y = map_landmarks.landmark_list[j].y_f;
		int m_id = map_landmarks.landmark_list[j].id_i;
		
		if (fabs(m_x - par_x) <= sensor_range && fabs(m_y - par_y) <= sensor_range){
			predictions.push_back(LandmarkObs{m_id, m_x, m_y});
		}
	 }
       	 
	 // Step 2: Transforms observations from vehicle coordinate to map coordinate
	 vector<LandmarkObs> transformed_os;
         for (int j = 0; j < observations.size(); j++){
      		double t_x = cos(par_theta) * observations[j].x - sin(par_theta) * observations[j].y + par_x;
      		double t_y = sin(par_theta) * observations[j].x + cos(par_theta) * observations[j].y + par_y;
      		transformed_os.push_back(LandmarkObs{observations[j].id, t_x, t_y });
    	 }
	 
	 // Step 3: Associate observations to landmarks using NN algorithm
 	 dataAssociation(predictions, transformed_os);
	 
	 // Step 4: Calculate the weight of each particle using Mutli V Gaussian dist 
         // Reset weights to 1.0
	 particles[i].weight = 1.0f;

         for (int j = 0; j < transformed_os.size(); j++){
          double obs_x, obs_y, pr_x, pr_y;
	  //double 
	  obs_x = transformed_os[j].x;
          //double 
	  obs_y = transformed_os[j].y;
	  //double pr_x, pr_y;
          
	  int associated_prediction = transformed_os[j].id;

          for (int k = 0; k < predictions.size(); k++){
           if (predictions[k].id == associated_prediction){
		pr_x = predictions[k].x;
           	pr_y = predictions[k].y;
          }   
         }

         double s_x = std_landmark[0];
         double s_y = std_landmark[1];
         double obs_w = (1 / (2 * M_PI * s_x * s_y)) * exp(-(pow(pr_x - obs_x, 2) / (2 * pow(s_x, 2)) + (pow(pr_y - obs_y,2) / (2 * pow(s_y, 2)))));

         particles[i].weight *= obs_w;
    }
  }
}

void ParticleFilter::resample() {
	// TODO: Resample particles with replacement with probability proportional to their weight. 
	// NOTE: You may find std::discrete_distribution helpful here.
	//   http://en.cppreference.com/w/cpp/numeric/random/discrete_distribution
	// default_random_engine gen;
	vector<Particle> tmp_particles; 
	
	// converting all weights into an array to symbolize circular wheel
	vector<double> weight_wheel;

	for (int i = 0; i < num_particles; i++){
		weight_wheel.push_back(particles[i].weight);
	}
 	
	//mw = max(w)
	double max_weight = *max_element(weight_wheel.begin(), weight_wheel.end());
	
	//index = int(random.random() * N)
	uniform_int_distribution<int> uniint(0, num_particles - 1);
	auto index = uniint(gen); //not double or int, because they both give a compilation error. No idea why?!
	
	double beta = 0.0; 

	uniform_real_distribution<double> unireal(0, max_weight);

	//for i in range(N): beta += random.random() * 2.0 * mw
    	//while beta > w[index]: beta -= w[index] index = (index + 1) % N p3.append(p[index])
		
	for (int i = 0; i < num_particles; i++){
    	 beta += unireal(gen) * 2.0;
    	 while (beta > weight_wheel[index]){
      	  beta -= weight_wheel[index];
      	  index = (index + 1) % num_particles;
    	 }
    	 tmp_particles.push_back(particles[index]);
  	}
  	
	//p = p3
	particles = tmp_particles;
	
}	


Particle ParticleFilter::SetAssociations(Particle& particle, const std::vector<int>& associations, 
                                     const std::vector<double>& sense_x, const std::vector<double>& sense_y)
{
    //particle: the particle to assign each listed association, and association's (x,y) world coordinates mapping to
    // associations: The landmark id that goes along with each listed association
    // sense_x: the associations x mapping already converted to world coordinates
    // sense_y: the associations y mapping already converted to world coordinates

    particle.associations= associations;
    particle.sense_x = sense_x;
    particle.sense_y = sense_y;
}

string ParticleFilter::getAssociations(Particle best)
{
	vector<int> v = best.associations;
	stringstream ss;
    copy( v.begin(), v.end(), ostream_iterator<int>(ss, " "));
    string s = ss.str();
    s = s.substr(0, s.length()-1);  // get rid of the trailing space
    return s;
}
string ParticleFilter::getSenseX(Particle best)
{
	vector<double> v = best.sense_x;
	stringstream ss;
    copy( v.begin(), v.end(), ostream_iterator<float>(ss, " "));
    string s = ss.str();
    s = s.substr(0, s.length()-1);  // get rid of the trailing space
    return s;
}
string ParticleFilter::getSenseY(Particle best)
{
	vector<double> v = best.sense_y;
	stringstream ss;
    copy( v.begin(), v.end(), ostream_iterator<float>(ss, " "));
    string s = ss.str();
    s = s.substr(0, s.length()-1);  // get rid of the trailing space
    return s;
}
