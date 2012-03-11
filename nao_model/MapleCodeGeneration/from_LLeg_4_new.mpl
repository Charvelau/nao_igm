# from LLeg (see Readme_M.txt and JacobianCoM.mpl)
#
# here I use all the joints
#

alias(fname = from_LLeg_4_new):

#---------------------------------------------------------------------
printf("form output ...\n");
st := time():

# ===================================================================================
# for convenience we use a homogeneous matrix to store the posture of the LLeg
# LL is used as an input parameter in the C function
# ===================================================================================
LL := vector(16): # Left leg transformation matrix <-- this is the support leg

q[28] := LL[1]: q[31] := LL[5]: q[34] :=  LL[9]: q[25] := LL[13]:
q[29] := LL[2]: q[32] := LL[6]: q[35] := LL[10]: q[26] := LL[14]:
q[30] := LL[3]: q[33] := LL[7]: q[36] := LL[11]: q[27] := LL[15]:
#        LL[4]:          LL[8]:          LL[12]:          LL[16]:
# ===================================================================================

# Form Jacobian matrices
J_RL_from_LL := FormJacobian(RLeg,LLeg):
J_Torso_from_LL := FormJacobian(Torso,LLeg):

JointFrames := FramesWorld(LLeg):
CM := SystemCoM(JointFrames):

# Associate an end-effector with the CoM of each link
# (because I don't want to change the function FormJacobianFull)
EE:=EE[1..6]: # leave only the first 6 in the list in case others have been added
EE := [op(EE), struct('parent',  0, 'r', TorsoLink:-r, 'e', < 0,0,0>)]:
for i from 1 to n do
    EE := [op(EE), struct('parent',  i, 'r', Links[i]:-r, 'e', < 0,0,0>)]:
end do:

m := 0:
J := Matrix(3,24,0):
for i from 0 to n do
    Je := FormJacobianFull(6+i+1, LLeg):

    if i=0 then
        J := J + TorsoLink:-m.Je[1..3,1..n]:
        m := m + TorsoLink:-m:
    else
        J := J + Links[i]:-m.Je[1..3,1..n]:
        m := m + Links[i]:-m:
    end if;

end do:
J := J/m: # Jacobian of the CoM

Jac := Matrix(11,24,0):
Jac[1...6,1..12] := J_RL_from_LL:
Jac[7..9,1..24] := J:
Jac[10,1..6] := J_Torso_from_LL[5,1..6]:
Jac[11,1..12] := <1|0|0|0|0|0|-1|0|0|0|0|0>: # constraint q[1] = q[7]

# FGM
RLegT := EEWorld(JointFrames,RLeg):
TorsoT := EEWorld(JointFrames,Torso):

RL := vector(16): # Right leg transformation matrix
err1 := FormError(convert(transpose(RLegT),vector), RL):

CoM := vector(3): # CoM position
CM := SystemCoM(JointFrames):
err2 := <CoM[1] - CM[1], CoM[2] - CM[2], CoM[3] - CM[3]>:

Rot := vector(9): # Torso rotation matrix
err3 := FormErrorRot(convert(transpose(TorsoT[1..3,1..3]),vector), Rot):

out := ArrayTools:-Concatenate(2,convert(transpose(Jac),vector),transpose(err1),transpose(err2),transpose(err3[2]),0):

printf("completed in %f seconds\n\n",time()-st);

#---------------------------------------------------------------------

printf("makeproc ...\n");
st := time():

fname := makeproc(out,[q::array(1..n), LL::array(1..linalg:-vectdim(LL)), RL::array(1..linalg:-vectdim(RL)), CoM::array(1..linalg:-vectdim(CoM)), Rot::array(1..linalg:-vectdim(Rot))]):

printf("completed in %f seconds\n\n",time()-st);

#---------------------------------------------------------------------

printf("Generating %s ...\n",cat(fname,".c"));
st := time():

fd := fopen(cat("../",cat(fname,".c")),WRITE):
fprintf(fd,"/* Generated using codegen (%s) */\n", StringTools:-FormatTime("%Y-%m-%d, %T")):
fclose(fd):

C(fname, optimized, filename = cat("../",cat(fname,".c")));

printf("completed in %f seconds\n\n",time()-st);

#---------------------------------------------------------------------