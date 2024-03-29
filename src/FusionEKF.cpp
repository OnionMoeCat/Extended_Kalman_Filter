#include "FusionEKF.h"
#include <iostream>
#include "Eigen/Dense"
#include "tools.h"

using Eigen::MatrixXd;
using Eigen::VectorXd;
using std::cout;
using std::endl;
using std::vector;

/**
 * Constructor.
 */
FusionEKF::FusionEKF() {
  is_initialized_ = false;

  previous_timestamp_ = 0;

  // initializing matrices
  R_laser_ = MatrixXd(2, 2);
  R_radar_ = MatrixXd(3, 3);
  H_laser_ = MatrixXd(2, 4);
  Hj_ = MatrixXd(3, 4);

  //measurement covariance matrix - laser
  R_laser_ << 0.0225, 0,
              0, 0.0225;

  //measurement covariance matrix - radar
  R_radar_ << 0.09, 0, 0,
              0, 0.0009, 0,
              0, 0, 0.09;

  H_laser_ << 1,0,0,0,
  			  0,1,0,0;

}

/**
 * Destructor.
 */
FusionEKF::~FusionEKF() {}

void FusionEKF::ProcessMeasurement(const MeasurementPackage &measurement_pack) {
  /**
   * Initialization
   */
  if (!is_initialized_) {

    // first measurement
    cout << "EKF: " << endl;
    ekf_.x_ = VectorXd(4);
    
    
  	// Initialize transition matrix F
  	ekf_.F_ = MatrixXd(4,4);

  	// Covariance matrix 
  	ekf_.P_ = MatrixXd(4,4);
    ekf_.P_ << 1,0,0,0,
               0,1,0,0,
               0,0,1000,0,
               0,0,0,1000;    
  	ekf_.Q_ = MatrixXd(4,4);
    
    if (measurement_pack.sensor_type_ == MeasurementPackage::RADAR) {
		float rho = measurement_pack.raw_measurements_(0);
		float phi = measurement_pack.raw_measurements_(1);

		float px = rho * cos(phi);
		float py = rho * sin(phi);

		ekf_.x_ << px, py, 0.0, 0.0;
    }
    else if (measurement_pack.sensor_type_ == MeasurementPackage::LASER) {
		ekf_.x_ << measurement_pack.raw_measurements_(0), measurement_pack.raw_measurements_(1), 0.0,0.0;
    }
	previous_timestamp_ = measurement_pack.timestamp_;
    // done initializing, no need to predict or update
    is_initialized_ = true;
    return;
  }

  /**
   * Prediction
   */

  float dt = (measurement_pack.timestamp_ - previous_timestamp_)/1000000.0;
  float dt_2 = dt * dt;
  float dt_3 = dt_2 * dt;
  float dt_4 = dt_3 * dt;
  constexpr float noise_ax = 9.0;
  constexpr float noise_ay = 9.0;

  previous_timestamp_ = measurement_pack.timestamp_;

  //Update the Q and F matrix

  ekf_.F_ << 1,0,dt,0,
             0,1,0,dt,
             0,0,1,0,
             0,0,0,1;
  ekf_.Q_ << dt_4/4*noise_ax, 0, dt_3/2*noise_ax, 0,
             0, dt_4/4*noise_ay, 0, dt_3/2*noise_ay,
             dt_3/2*noise_ax, 0, dt_2*noise_ax, 0,
             0, dt_3/2*noise_ay, 0, dt_2*noise_ay;

  ekf_.Predict();

  /**
   * Update
   */

  if (measurement_pack.sensor_type_ == MeasurementPackage::RADAR) {
	// Radar updates
    float rho = measurement_pack.raw_measurements_(0);
	float phi = measurement_pack.raw_measurements_(1);
	float rhodot = measurement_pack.raw_measurements_(2);
	VectorXd z_Radar(3);
	z_Radar << rho, phi, rhodot;
    Tools t;
    ekf_.H_ = t.CalculateJacobian(ekf_.x_);
    ekf_.R_ = R_radar_;
    ekf_.UpdateEKF(z_Radar);
  } else {
	VectorXd z_Laser(2);
	z_Laser << measurement_pack.raw_measurements_(0), measurement_pack.raw_measurements_(1);
    ekf_.H_ = H_laser_ ; // H matrix for the case of laser
	ekf_.R_ = R_laser_ ;// measurement covariance matrix
    ekf_.Update(z_Laser); 
  }

  // print the output
  cout << "x_ = " << ekf_.x_ << endl;
  cout << "P_ = " << ekf_.P_ << endl;
}
