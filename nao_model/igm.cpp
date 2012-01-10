#include <math.h> // sqrt

#include "nao_igm.h"
#include "maple_functions.h"

#include <Eigen/Core>   
#include <Eigen/Cholesky>


/**
 * @brief Constructor
 */
nao_igm::nao_igm()
{
    state_var_num = JOINTS_NUM + SUPPORT_FOOT_POS_NUM + SUPPORT_FOOT_ORIENTATION_NUM;

    // LEFT LEG
    setBounds(L_HIP_YAW_PITCH , -1.145303,  0.740810);
    setBounds(L_HIP_ROLL      , -0.379472,  0.790477);
    setBounds(L_HIP_PITCH     , -1.773912,  0.484090);
    setBounds(L_KNEE_PITCH    , -0.092346,  2.112528);
    setBounds(L_ANKLE_PITCH   , -1.189516,  0.922747);
    setBounds(L_ANKLE_ROLL    , -0.769001,  0.397880);
                              
    // RIGHT LEG
    setBounds(R_HIP_YAW_PITCH , -1.145303,  0.740810);
    setBounds(R_HIP_ROLL      , -0.738321,  0.414754);
    setBounds(R_HIP_PITCH     , -1.772308,  0.485624);
    setBounds(R_KNEE_PITCH    , -0.103083,  2.120198);
    setBounds(R_ANKLE_PITCH   , -1.186448,  0.932056);
    setBounds(R_ANKLE_ROLL    , -0.388676,  0.785875);
                              
    // LEFT ARM
    setBounds(L_SHOULDER_PITCH, -2.085600,  2.085600);
    setBounds(L_SHOULDER_ROLL ,  0.008700,  1.649400);
    setBounds(L_ELBOW_YAW     , -2.085600,  2.085600);
    setBounds(L_ELBOW_ROLL    , -1.562100, -0.008700);
    setBounds(L_WRIST_YAW     , -1.823800,  1.823800);
                              
    // RIGHT ARM
    setBounds(R_SHOULDER_PITCH, -2.085600,  2.085600);
    setBounds(R_SHOULDER_ROLL , -1.649400, -0.008700);
    setBounds(R_ELBOW_YAW     , -2.085600,  2.085600);
    setBounds(R_ELBOW_ROLL    ,  0.008700,  1.562100);
    setBounds(R_WRIST_YAW     , -1.823800,  1.823800);
                              
    // HEAD                   
    setBounds(HEAD_PITCH      , -2.085700,  2.085700);
    setBounds(HEAD_YAW        , -0.672000,  0.514900);

    initJointAngles();
}



/**
 * @brief Set bounds for a joint.
 *
 * @param[in] id id of the joint.
 * @param[in] lower_bound lower bound.
 * @param[in] upper_bound upper_bound.
 */
void nao_igm::setBounds (const jointSensorIDs id, const double lower_bound, const double upper_bound)
{
    q_lower_bound[id] = lower_bound;
    q_upper_bound[id] = upper_bound;
}



/**
 * @brief Check that all joint angles lie within bounds.
 *
 * @return -1 if all values are corrent, id of the first joint violating the
 * bounds.
 *
 * @attention No collision checks!
 */
int nao_igm::checkJointBounds()
{
    for (int i = 0; i < JOINTS_NUM; i++)
    {
        if ((q_lower_bound[i] > q[i]) || (q_upper_bound[i] < q[i]))
        {
            return (i);
        }
    }
    return (-1);
}



/** \brief Sets the pose of the base (of NAO)

    \param[in] x x-position
    \param[in] y y-position
    \param[in] z z-position
    \param[in] alpha x-rotation
    \param[in] beta y-rotation
    \param[in] gamma z-rotation
*/
void nao_igm::SetBasePose(
        const double x, 
        const double y, 
        const double z, 
        const double alpha, 
        const double beta, 
        const double gamma)
{
    double Rot[3*3];

    Euler2Rot(alpha, beta, gamma, Rot);
    q[24] = x;
    q[25] = y;
    q[26] = z;

    q[27] = Rot[0];
    q[30] = Rot[3];
    q[33] = Rot[6];
    q[28] = Rot[1];
    q[31] = Rot[4];
    q[34] = Rot[7];
    q[29] = Rot[2];
    q[32] = Rot[5];
    q[35] = Rot[8];
}



/** \brief Given a posture of a frame (specified using a 4x4 homogeneous matrix Tc) and an offset
    (x, y, z, X(alpha), Y(beta), Z(gamma)) returns a posture Td that includes the offset.

    \param[in] Tc 4x4 homogeneous matrix
    \param[out] Td 4x4 homogeneous matrix

    \param[in] x x-position offset
    \param[in] y y-position offset
    \param[in] z z-position offset
    \param[in] alpha x-rotation offset
    \param[in] beta y-rotation offset
    \param[in] gamma z-rotation offset
*/
void nao_igm::PostureOffset(
        const double *Tc, 
        double *Td,
        const double x, 
        const double y, 
        const double z, 
        const double alpha, 
        const double beta, 
        const double gamma)
{
    double tmp[4*4];
    Euler2T(x, y, z, alpha, beta, gamma, tmp);

    Eigen::Map<Eigen::MatrixXd> TcE(Tc,4,4);
    Eigen::Map<Eigen::MatrixXd> T(tmp,4,4);

    T = TcE*T;

    for (int i=0; i<4*4; i++)
        Td[i] = T(i);
}



/**
 * @brief Set position in a 4x4 homogeneous matrix.
 *
 * @param[in,out] Tc 4x4 homogeneous matrix.
 * @param[in] position 3x1 vector of coordinates.
 * @param[in] roll angle
 * @param[in] pitch angle
 * @param[in] yaw angle
 */
void nao_igm::initPosture (
        double *Tc, 
        const double *position,
        const double roll,
        const double pitch,
        const double yaw)
{
    Tc[12] = position[0];
    Tc[13] = position[1];
    Tc[14] = position[2];

    // form the rotation matrix corresponding to a set of roll-pitch-yaw angles
    rpy2R_hom (roll, pitch, yaw, Tc);
}


/**
 * @brief Set coordinates of center of mass.
 *
 * @param[in] x coordinate
 * @param[in] y coordinate
 * @param[in] z coordinate
 */
void nao_igm::setCoM (const double x, const double y, const double z)
{
    CoM_position[0] = x;
    CoM_position[1] = y;
    CoM_position[2] = z;
}


/**
 * @brief Update CoM after joint angles were changed and return result.
 *
 * @param[in,out] CoM_pos 3x1 vector of coordinates.
 */
void nao_igm::getUpdatedCoM (double *CoM_pos)
{
    if (support_foot == IGM_SUPPORT_LEFT)
    {
        LLeg2CoM(q, CoM_position);
    }
    else
    {
        RLeg2CoM(q, CoM_position);
    }

    CoM_pos[0] = CoM_position[0];
    CoM_pos[1] = CoM_position[1];
    CoM_pos[2] = CoM_position[2];
}



/**
 * @brief Update swing foot position after joint angles were changed and return result.
 *
 * @param[in,out] swing_foot  3x1 vector of coordinates.
 */
void nao_igm::getUpdatedSwingFoot (double * swing_foot)
{
    if (support_foot == IGM_SUPPORT_LEFT)
    {
        LLeg2RLeg(q, swing_foot_posture);
    }
    else
    {
        RLeg2LLeg(q, swing_foot_posture);
    }

    swing_foot[0] = swing_foot_posture[12];
    swing_foot[1] = swing_foot_posture[13];
    swing_foot[2] = swing_foot_posture[14];
}



/** \brief Given a rotation matrix and an offset specified as X(alpha)->Y(beta)->Z(gamma) (current
    axis) Euler angles, returns a rotation matrix Rd that includes the offset.

    \param[in] Rc 4x4 rotation matrix
    \param[out] Rd 4x4 rotation matrix

    \param[in] alpha x-rotation offset
    \param[in] beta y-rotation offset
    \param[in] gamma z-rotation offset
*/
void nao_igm::RotationOffset(
        const double *Rc, 
        double *Rd, 
        const double alpha, 
        const double beta, 
        const double gamma)
{
    double tmp[3*3];

    Euler2Rot(alpha, beta, gamma, tmp);

    Eigen::Map<Eigen::MatrixXd> RcE(Rc,3,3);
    Eigen::Map<Eigen::MatrixXd> R(tmp,3,3);

    R = RcE*R;

    for (int i=0; i<3*3; i++)
        Rd[i] = R(i);
}



/**
 * @brief Initialize model.
 *
 * @param[in] support_foot_ current support foot.
 * @param[in] sup_position position of the support foot.
 * @param[in] sup_orientation orientation of the supprt foot.
 */
void nao_igm::init(
        const igmSupportFoot support_foot_, 
        const double *sup_position, 
        const double *sup_orientation)
{
    for (int i = SUPPORT_FOOT_POS_START; 
            i < SUPPORT_FOOT_POS_START + SUPPORT_FOOT_POS_NUM; 
            i++)
    {
        q[i] = sup_position[i - SUPPORT_FOOT_POS_START];
    }

    for (int i = SUPPORT_FOOT_ORIENTATION_START; 
            i < SUPPORT_FOOT_ORIENTATION_START + SUPPORT_FOOT_ORIENTATION_NUM; 
            i++)
    {
        q[i] = sup_orientation[i - SUPPORT_FOOT_ORIENTATION_START];
    }


    support_foot = support_foot_;
    double torso_posture[POSTURE_MATRIX_SIZE];
    if (support_foot == IGM_SUPPORT_LEFT)
    {
        LLeg2RLeg(q, swing_foot_posture);
        LLeg2Torso(q, torso_posture);
        LLeg2CoM(q, CoM_position);
    }
    else
    {
        RLeg2LLeg(q, swing_foot_posture);
        RLeg2Torso(q, torso_posture);
        RLeg2CoM(q, CoM_position);
    }
    T2Rot(torso_posture, torso_orientation);
}


/**
 * @brief Switch support foot.
 */
void nao_igm::switchSupportFoot()
{
    if (support_foot == IGM_SUPPORT_LEFT)
    {
        support_foot = IGM_SUPPORT_RIGHT;
    }
    else
    {
        if (support_foot == IGM_SUPPORT_RIGHT)
        {
            support_foot = IGM_SUPPORT_LEFT;
        }
    }

    for (int i = SUPPORT_FOOT_POS_START; 
            i < SUPPORT_FOOT_POS_START + SUPPORT_FOOT_POS_NUM; 
            i++)
    {
        q[i] = swing_foot_posture[12 + i - SUPPORT_FOOT_POS_START];
    }

    double sup_orientation[ORIENTATION_MATRIX_SIZE];
    T2Rot (swing_foot_posture, sup_orientation);
    for (int i = SUPPORT_FOOT_ORIENTATION_START; 
            i < SUPPORT_FOOT_ORIENTATION_START + SUPPORT_FOOT_ORIENTATION_NUM; 
            i++)
    {
        q[i] = sup_orientation[i - SUPPORT_FOOT_ORIENTATION_START];
    }


    double torso_posture[POSTURE_MATRIX_SIZE];
    if (support_foot == IGM_SUPPORT_LEFT)
    {
        LLeg2RLeg(q, swing_foot_posture);
        LLeg2Torso(q, torso_posture);
        LLeg2CoM(q, CoM_position);
    }
    else
    {
        RLeg2LLeg(q, swing_foot_posture);
        RLeg2Torso(q, torso_posture);
        RLeg2CoM(q, CoM_position);
    }
    T2Rot(torso_posture, torso_orientation);
}



/** \brief Sets the initial configuration of nao (lets call it the standard initial configuration)

    \note Only q[0]...q[23] are set. The posture of the base is not set.
*/
void nao_igm::initJointAngles()
{
    // LEFT LEG
    q[L_HIP_YAW_PITCH] =  0.0;
    q[L_HIP_ROLL]      =  0.0;
    q[L_HIP_PITCH]     = -0.436332;
    q[L_KNEE_PITCH]    =  0.698132;
    q[L_ANKLE_PITCH]   = -0.349066;
    q[L_ANKLE_ROLL]    =  0.0;

    // RIGHT LEG
    q[R_HIP_YAW_PITCH] =  0.0;
    q[R_HIP_ROLL]      =  0.0;
    q[R_HIP_PITCH]     = -0.436332;
    q[R_KNEE_PITCH]    =  0.698132;
    q[R_ANKLE_PITCH]   = -0.349066;
    q[R_ANKLE_ROLL]    =  0.0;

    // LEFT ARM
    q[L_SHOULDER_PITCH] =  1.396263;
    q[L_SHOULDER_ROLL]  =  0.349066;
    q[L_ELBOW_YAW]      = -1.396263;
    q[L_ELBOW_ROLL]     = -1.047198;
    q[L_WRIST_YAW]      =  0.0;

    // RIGHT ARM
    q[R_SHOULDER_PITCH] =  1.396263;
    q[R_SHOULDER_ROLL]  = -0.349066;
    q[R_ELBOW_YAW]      =  1.396263;
    q[R_ELBOW_ROLL]     =  1.047198;
    q[R_WRIST_YAW]      =  0.0;

    // HEAD
    q[HEAD_PITCH] =  0.0;
    q[HEAD_YAW]   =  0.0;
}



/** \brief Extracts the rotation matrix from a 4x4 homogeneous matrix

    \param[in] T 4x4 homogeneous matrix.
    \param[out] Rot 3x3 rotation matrix
*/
void nao_igm::T2Rot(const double * T, double *Rot)
{
    Rot[0] = T[0];
    Rot[3] = T[4];
    Rot[6] = T[8];
    Rot[1] = T[1];
    Rot[4] = T[5];
    Rot[7] = T[9];
    Rot[2] = T[2];
    Rot[5] = T[6];
    Rot[8] = T[10];
}


/** \brief Forms the rotation matrix corresponding to a set of roll-pitch-yaw angles

    \param[in] roll roll angle [radian]
    \param[in] pitch pitch angle [radian]
    \param[in] yaw yaw angle [radian]
    \param[out] R Rotation matrix corresponding to the roll-pitch-yaw angles
    
    \note The rotation defined using roll, pitch and yaw angles is assumed to be formed by first
    applying a rotation around the x axis (roll), then a rotation around the new y axis (pitch) and
    finally a rotation around the new z axis (yaw).

    \note The matrix is stored column-wise (Fortran formatting)
 */
void nao_igm::rpy2R(const double roll, const double pitch, const double yaw, double *R)
{
    double sr = sin(roll);
    double cr = cos(roll);

    double sp = sin(pitch);
    double cp = cos(pitch);

    double sy = sin(yaw);
    double cy = cos(yaw);

    R[0] =  cp*cy;            R[3] = -cp*sy;            R[6] =  sp;
    R[1] =  sr*sp*cy + cr*sy; R[4] = -sr*sp*sy + cr*cy; R[7] = -sr*cp;
    R[2] = -cr*sp*cy + sr*sy; R[5] =  cr*sp*sy + sr*cy; R[8] =  cr*cp;
}


/** \brief Forms the rotation matrix corresponding to a set of roll-pitch-yaw angles

    \param[in] roll roll angle [radian]
    \param[in] pitch pitch angle [radian]
    \param[in] yaw yaw angle [radian]
    \param[out] R homogeneous matrix
    
    \note The rotation defined using roll, pitch and yaw angles is assumed to be formed by first
    applying a rotation around the x axis (roll), then a rotation around the new y axis (pitch) and
    finally a rotation around the new z axis (yaw).

    \note The matrix is stored column-wise (Fortran formatting)
 */
void nao_igm::rpy2R_hom(const double roll, const double pitch, const double yaw, double *R)
{
    double sr = sin(roll);
    double cr = cos(roll);

    double sp = sin(pitch);
    double cp = cos(pitch);

    double sy = sin(yaw);
    double cy = cos(yaw);

    R[0] =  cp*cy;            R[4] = -cp*sy;            R[8] =  sp;
    R[1] =  sr*sp*cy + cr*sy; R[5] = -sr*sp*sy + cr*cy; R[9] = -sr*cp;
    R[2] = -cr*sp*cy + sr*sy; R[6] =  cr*sp*sy + sr*cy; R[10] =  cr*cp;
}
