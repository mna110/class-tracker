#include "ekf.h"

void State::print()
{
    std::cout << "x: " << x_ << "\ty: " << y_ << "\tyaw: " << yaw_ << "\tv: " << vel_ << "\tyaw rate: " << yaw_rate_ << std::endl;
}

EKF::EKF() {}

EKF::EKF(const int n_states, const float dt, const EKFMatrixF &Q, const EKFMatrixF &R, const State &in_state)
{

    n_states_ = n_states;
    dt_ = dt;

    Q_ = Q;
    R_ = R;
    P_ = EKFMatrixF::Identity(n_states_, n_states_);
    H_ = EKFMatrixF::Identity(n_states_, n_states_);

    x_est_ = in_state;
}

EKF::~EKF()
{
    //FIXME - undestand why segfault
    /* if (Q_)
        delete Q_;
    if (R_)
        delete R_;
    if (H_)
        delete H_;
    if (P_)
        delete P_; */
}

void EKF::printInternalState()
{
    std::cout << "***************Internal state*********************" << std::endl;
    std::cout << "n_states: " << n_states_ << std::endl;
    std::cout << "dt_: " << dt_ << std::endl;
    std::cout << "P: \n"
              << P_ << std::endl;
    std::cout << "Q: \n"
              << Q_ << std::endl;
    std::cout << "R: \n"
              << R_ << std::endl;
    std::cout << "H: \n"
              << H_ << std::endl;
    x_est_.print();
    std::cout << "**************************************************" << std::endl;
}

Eigen::VectorXf StateIntoVector(const State &x, int n_states)
{
    Eigen::VectorXf v(n_states);

    v(0) = x.x_;
    v(1) = x.y_;
    v(2) = x.yaw_;
    v(3) = x.vel_;
    v(4) = x.yaw_rate_;
    return v;
}

State VectorIntoState(const Eigen::VectorXf &v)
{
    State x(v(0), v(1), v(2), v(3), v(4));
    return x;
}

// State x;
// EKF::EKFMatrixF J, PPred, Ht, S, K;
// Eigen::VectorXf y, x_est_vec;

void EKF::EKFStep(const EKFMatrixF &H, const Eigen::VectorXf &z)
{
    H_ = H;

    //predict
    State x = StateTransition();
    EKFMatrixF J = Jacobian(x);
    EKFMatrixF PPred = J * P_ * J.transpose() + Q_;

    //update

    EKFMatrixF Ht = H_.transpose();

    Eigen::VectorXf y = z - StateIntoVector(x_est_, n_states_);
    EKFMatrixF S = H_ * PPred * Ht + R_;
    EKFMatrixF K = PPred * Ht * (S.inverse());
    Eigen::VectorXf x_est_vec = StateIntoVector(x, n_states_) + K * y;
    x_est_ = VectorIntoState(x_est_vec);
    P_ = PPred - K * H_ * PPred;
}

State EKF::StateTransition()
{
    State x = x_est_;
    State y;
    if (abs(x.yaw_rate_) < 0.0001)
    {
        y.x_ = x.x_ + x.vel_ * dt_ * cos(x.yaw_);
        y.y_ = x.y_ + x.vel_ * dt_ * sin(x.yaw_);
        y.yaw_ = x.yaw_;
        y.vel_ = x.vel_;
        y.yaw_rate_ = 0.0001;
    }
    else
    {
        y.x_ = x.x_ + (x.vel_ / x.yaw_rate_) * (sin(x.yaw_rate_ * dt_ + x.yaw_) - sin(x.yaw_));
        y.y_ = x.y_ + (x.vel_ / x.yaw_rate_) * (-cos(x.yaw_rate_ * dt_ + x.yaw_) + cos(x.yaw_));
        y.yaw_ = x.yaw_ + x.yaw_rate_ * dt_;
        y.vel_ = x.vel_;
        y.yaw_rate_ = x.yaw_rate_;
    }

    return y;
}

EKF::EKFMatrixF EKF::Jacobian(const State &x)
{

    EKFMatrixF J(n_states_, n_states_);
    J.setIdentity();

    J(0, 2) = (x.vel_ / x.yaw_rate_) * (cos(x.yaw_rate_ * dt_ + x.yaw_) - cos(x.yaw_));
    J(0, 3) = (1.0 / x.yaw_rate_) * (sin(x.yaw_rate_ * dt_ + x.yaw_) - sin(x.yaw_));
    J(0, 4) = (dt_ * x.vel_ / x.yaw_rate_) * cos(x.yaw_rate_ * dt_ + x.yaw_) - (x.vel_ / pow(x.yaw_rate_, 2)) * (sin(x.yaw_rate_ * dt_ + x.yaw_) - sin(x.yaw_));
    J(1, 2) = (x.vel_ / x.yaw_rate_) * (sin(x.yaw_rate_ * dt_ + x.yaw_) - sin(x.yaw_));
    J(1, 3) = (1.0 / x.yaw_rate_) * (-cos(x.yaw_rate_ * dt_ + x.yaw_) + cos(x.yaw_));
    J(1, 4) = (dt_ * x.vel_ / x.yaw_rate_) * sin(x.yaw_rate_ * dt_ + x.yaw_) - (x.vel_ / pow(x.yaw_rate_, 2)) * (-cos(x.yaw_rate_ * dt_ + x.yaw_) + cos(x.yaw_));
    J(2, 4) = dt_;

    return J;
}

State EKF::getEstimatedState()
{
    return x_est_;
}
