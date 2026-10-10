//
// ********************************************************************
// * License and Disclaimer                                           *
// *                                                                  *
// * The  Geant4 software  is  copyright of the Copyright Holders  of *
// * the Geant4 Collaboration.  It is provided  under  the terms  and *
// * conditions of the Geant4 Software License,  included in the file *
// * LICENSE and available at  http://cern.ch/geant4/license .  These *
// * include a list of copyright holders.                             *
// *                                                                  *
// * Neither the authors of this software system, nor their employing *
// * institutes,nor the agencies providing financial support for this *
// * work  make  any representation or  warranty, express or implied, *
// * regarding  this  software system or assume any liability for its *
// * use.  Please see the license in the file  LICENSE  and URL above *
// * for the full disclaimer and the limitation of liability.         *
// *                                                                  *
// * This  code  implementation is the result of  the  scientific and *
// * technical work of the GEANT4 collaboration.                      *
// * By using,  copying,  modifying or  distributing the software (or *
// * any work based  on the software)  you  agree  to acknowledge its *
// * use  in  resulting  scientific  publications,  and indicate your *
// * acceptance of all terms of the Geant4 Software license.          *
// ********************************************************************
//
// 
/// \file B4PrimaryGeneratorAction.cc
/// \brief Implementation of the B4PrimaryGeneratorAction class

#include "B4PrimaryGeneratorAction.hh"
#include "B4Analysis.hh"

#include "B4cConstants.hh"

#include "G4RunManager.hh"
#include "G4LogicalVolumeStore.hh"
#include "G4LogicalVolume.hh"
#include "G4Box.hh"
#include "G4Event.hh"
#include "G4ParticleGun.hh"
#include "G4ParticleTable.hh"
#include "G4ParticleDefinition.hh"
#include "G4SystemOfUnits.hh"
#include "Randomize.hh"
#include "G4PhysicalConstants.hh"
#include "B4RunAction.hh"

//....oooOO0OOooo........oooOO0OOooo........oooOO0OOooo........oooOO0OOooo......

//>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>
// to plot distributions:
//dN/d(cos(theta) vs theta : 
//   B4->Draw("dth","weightE0*0.0001/(1.8*sin(dth*3.14/180))")
//
//dN/d(theta) vs theta:
//   B4->Draw("dth*3.14/180","weightE0*0.0001/1.8")
//
//dN/dcos(theta) vs cos(theta):
//   B4->Draw("cos(dth*3.14/180)","weightE0*0.0001/1.8")
//<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<

B4PrimaryGeneratorAction::B4PrimaryGeneratorAction()
 : G4VUserPrimaryGeneratorAction(),
   fParticleGun(nullptr),
   fParticleGun2(nullptr)
{
  G4int nofParticles = 1;
  fParticleGun = new G4ParticleGun(nofParticles);

  // default particle kinematic
  //
  auto particleDefinition 
    = G4ParticleTable::GetParticleTable()->FindParticle("mu+");
  fParticleGun->SetParticleDefinition(particleDefinition);

  fParticleGun->SetParticleMomentumDirection(G4ThreeVector(0.0,1.));

  fParticleGun->SetParticleEnergy(6.*MeV);
}

//....oooOO0OOooo........oooOO0OOooo........oooOO0OOooo........oooOO0OOooo......

B4PrimaryGeneratorAction::~B4PrimaryGeneratorAction()
{
  delete fParticleGun;
}

//....oooOO0OOooo........oooOO0OOooo........oooOO0OOooo........oooOO0OOooo......

G4double PolyLog2(G4double x){
  G4double t2=10;
  G4int n=2;
  G4double y=x;
  for(int k=2; k<5; k++){
    t2=exp(k*log(x)-n*log(k)); // x**k/k**n
    y=y+t2;
    //    std::cout << " x =" << x << "  t2 = " << t2 <<  " y = " << y << std::endl;
  }
  return y;
}

G4double Elliptic1(G4double hk){
  G4double ak;
  G4double bk;
  G4double ck;
  G4double pk;

  pk = 1.0 - hk * hk;

  if ( hk== 1.0){
    ck = 1.0E300;
  }
  else{
        ak = ((( 0.01451196212* pk + 0.03742563713 ) * pk + 0.03590092383 ) * pk + 0.09666344259 ) * pk + 1.38629436112;
        bk = ((( 0.00441787012 * pk + 0.03328355346 ) * pk + 0.06880248576 ) * pk + 0.12498593597 ) * pk + 0.5;
        ck = ak - bk * log ( pk );
  }
  return ck;
}
G4double Elliptic2(G4double hk){
  G4double ae;
  G4double be;
  G4double ce;
  G4double pk;

  pk = 1.0 - hk * hk;

  if ( hk== 1.0){
    ce = 1.0;
  }
  else{
        ae = ((( 0.01736506451 * pk + 0.04757383546 ) * pk + 0.0626060122 ) * pk + 0.44325141463 ) * pk + 1.0;
        be = ((( 0.00526449639 * pk + 0.04069697526 ) * pk + 0.09200180037) * pk + 0.2499836831  ) * pk;
        ce = ae - be * log ( pk );
  }
  return ce;
}

//....oooOO0OOooo........oooOO0OOooo........oooOO0OOooo........oooOO0OOooo......

void B4PrimaryGeneratorAction::GeneratePrimaries(G4Event* anEvent)
{
  // This function is called at the begining of event
 
  //
  //  particle kinematic
  //
 
  G4int nofParticles = 1;
  auto particleDefinition 
    //    = G4ParticleTable::GetParticleTable()->FindParticle("e-");
    = G4ParticleTable::GetParticleTable()->FindParticle("e+");
  auto particleDefinition2 
    = G4ParticleTable::GetParticleTable()->FindParticle("e-");
  auto particleDefinition3 
    = G4ParticleTable::GetParticleTable()->FindParticle("gamma");
  auto particleDefinition4 
    = G4ParticleTable::GetParticleTable()->FindParticle("mu+");
  
  
  G4int fsboson = sboson;
  if(fsboson > 0)

    //  +++++++++++++++++  boson  +++++++++++++++++++++++++++++++++++

    {  
      // Generate e+ and e- 4-vectors from boson.
      
      G4double mass_boson = 17.;
      //      G4double En = 18.34; // Total energy of boson for 8Be <============= proton beam of 1225 MeV
      G4double En = 19.7; // Total energy of boson for 8Be <=============
      //      G4double En = 18.15; // Total energy of boson for 8Be
      //      G4double En = 18.857; // Total energy of boson (12C, with E_p = 2.90 MeV)
      //      G4double En = 19.28; // Total energy of boson for Boron gamma_3
      //      G4double En = 18.4; // Total energy of boson for Boron gamma_3
      G4double p_boson = sqrt(En*En-mass_boson*mass_boson); // Momentum of the boson
      G4double costh_b = -1. + 2.*G4UniformRand(); // cos(theta) value of the  boson [-1, 1]
      //      G4double costh_b = 0.; // perpendicular   <------------------------
      G4double sinth_b = sqrt(1.-costh_b*costh_b); // sin(theta) value of the boson [-1, 1]
      G4double phi_b = 2*pi*G4UniformRand(); // Phi angle of the boson [0, 2pi]
      
      
      // In center of mass frame
      
      G4double w_pcm = mass_boson/2.; // positron energy
      G4double w_mcm = mass_boson/2.; // electron energy
      G4double p_pcm = sqrt(w_pcm*w_pcm-0.511*0.511); // Momentum of positron
      G4double costh_pcm = -1. + 2*G4UniformRand(); // cos(theta) value of positron [-1, 1]
      //      G4double costh_pcm = 0.; // perpendicular  <-----------------
      G4double sinth_pcm = sqrt(1. -costh_pcm*costh_pcm); // sin(theta) value of positron [-1, 1]
      G4double phi_pcm = 2*pi*G4UniformRand(); // Phi angle of prositron [0, 2pi]
      G4double pz_pcm = p_pcm*costh_pcm; // z component of momentum of positron
      G4double px_pcm = p_pcm*sinth_pcm*cos(phi_pcm); // x component of momentum of positron
      G4double py_pcm = p_pcm*sinth_pcm*sin(phi_pcm); // y component of momentum of positron
      
      
      // Momentum and 4-momentum vectors
      
      G4ThreeVector pos3_cm = G4ThreeVector(px_pcm, py_pcm, pz_pcm);  // positron momentum in frame of boson
      G4LorentzVector pos_cm = G4LorentzVector(pos3_cm, w_pcm);  // positron 4-momentum frame of boson
      G4ThreeVector elec3_cm = G4ThreeVector(-px_pcm, -py_pcm, -pz_pcm); // electron momentum frame of boson
      G4LorentzVector elec_cm = G4LorentzVector(elec3_cm, w_mcm); // electron 4-momentum frame of boson
      
      
      // Boost given to electron and positron.
      
      G4ThreeVector boostv = G4ThreeVector(p_boson*sinth_b*cos(phi_b)/En, 
					   p_boson*sinth_b*sin(phi_b)/En,
					   p_boson*costh_b/En);   // boost vector = beta components
      G4LorentzVector pos = pos_cm.boost(boostv);  // boosted positron 4-vector in the lab
      G4LorentzVector elec = elec_cm.boost(boostv);  // boosted electron 4-vector in the lab
      
      
      // Values of positrons in the lab frame
      G4double px_p = pos.px();
      G4double py_p = pos.py();
      G4double pz_p = pos.pz();
      G4double w_p = pos.e();
      G4double p_p = sqrt(px_p*px_p + py_p*py_p+pz_p*pz_p);
      G4double costh_p = pos.cosTheta();
      G4double phi_p = atan2(py_p,px_p);
      
      // Values of electrons in the lab frame
      G4double px_m = elec.px();
      G4double py_m = elec.py();
      G4double pz_m = elec.pz();
      G4double w_m = elec.e();
      G4double p_m = sqrt(px_m*px_m + py_m*py_m+pz_m*pz_m);  
      G4double costh_m = elec.cosTheta();
      G4double phi_m = atan2(py_m,px_m);
      
      // Units changing
      G4double ak = En/0.511; // energy in units of electron mass
      w_p /= 0.511;   // in units of electron mass
      w_m /= 0.511;
      
      // Change the components in unit vectors
      G4double pxU_p = px_p/p_p;
      G4double pyU_p = py_p/p_p;
      G4double pzU_p = pz_p/p_p;
      G4double pxU_m = px_m/p_m;
      G4double pyU_m = py_m/p_m;
      G4double pzU_m = pz_m/p_m;
      G4double cos_dth = (pxU_p*pxU_m + pyU_p*pyU_m + pzU_p*pzU_m);  // cos of opening angle
      
      // Angle between the boson and electron/positron.
      G4double pxU_b = sinth_b*cos(phi_b);
      G4double pyU_b = sinth_b*sin(phi_b);
      G4double pzU_b = costh_b;
      G4double angle1 = acos(pxU_b*pxU_m + pyU_b*pyU_m + pzU_b*pzU_m)*180./pi; // for electron
      G4double angle2 = acos(pxU_b*pxU_p + pyU_b*pyU_p + pzU_b*pzU_p)*180./pi; // for positron
      
      
      // Mass energy of the electrons.
      G4double m_ee = sqrt( pow((w_m + w_p)*0.511,2) 
			    - pow(px_p+px_m, 2) - pow(py_p+py_m, 2)- pow(pz_p+pz_m, 2));
      
      // Asymmetry of the energy.
      G4double asymmetry = (w_p - w_m)/(w_p + w_m - 2);
      
      
      //
      // ---------------------------------------------------------------
      //
      
      

      G4double weightRoseE1 = 1.;
      G4double weightStieblingE1 = 1.;
      G4double   weightRoseM1 =1.;
      G4double   weightStieblingM1  = 1.;
      G4double   weightE0 = 1.;
      
      
      // // --------------------------------------------------------------------------------------
      
      fParticleGun->SetNumberOfParticles(nofParticles);  
      
      fParticleGun->SetParticleDefinition(particleDefinition);
      fParticleGun->SetParticleMomentumDirection(G4ThreeVector(pxU_p, pyU_p, costh_p));
      fParticleGun->SetParticleEnergy((w_p-1.)*0.511);
      fParticleGun->SetParticlePosition(G4ThreeVector(0., 0., 0.*cm));
      //  fParticleGun->SetParticlePosition(G4ThreeVector(0.3*cm, 0., 0.*cm));  // <-------------
      fParticleGun->GeneratePrimaryVertex(anEvent);
      
      fParticleGun->SetParticleDefinition(particleDefinition2);
      fParticleGun->SetParticleMomentumDirection(G4ThreeVector(pxU_m, pyU_m, costh_m));
      fParticleGun->SetParticleEnergy((w_m-1.)*0.511);
      fParticleGun->SetParticlePosition(G4ThreeVector(0., 0., 0.*cm)); 
      //  fParticleGun->SetParticlePosition(G4ThreeVector(0.3*cm, 0., 0.*cm));  // <-------------
      fParticleGun->GeneratePrimaryVertex(anEvent);
      
      // get analysis manager
      auto analysisManager = G4AnalysisManager::Instance();
      
      // fill histograms
      analysisManager->FillH1(1, costh_p);
      analysisManager->FillH1(2, phi_p);
      analysisManager->FillH1(3, costh_m);
      analysisManager->FillH1(4, phi_m);
      
      // fill ntuple
      analysisManager->FillNtupleDColumn(1, costh_p);
      analysisManager->FillNtupleDColumn(2, phi_p);
      analysisManager->FillNtupleDColumn(3, costh_m);
      analysisManager->FillNtupleDColumn(4, phi_m);
      analysisManager->FillNtupleDColumn(5, ak*0.511);
      analysisManager->FillNtupleDColumn(6, (w_p-1.)*0.511);
      analysisManager->FillNtupleDColumn(7, (w_m-1)*0.511);
      analysisManager->FillNtupleDColumn(8, acos(cos_dth)*180./pi);
      analysisManager->FillNtupleDColumn(9, asymmetry);
      analysisManager->FillNtupleDColumn(10, m_ee);
      analysisManager->FillNtupleDColumn(11, angle1);
      analysisManager->FillNtupleDColumn(12, angle2);
      analysisManager->FillNtupleDColumn(13, weightRoseE1);
      //      analysisManager->FillNtupleDColumn(14, weightStieblingE1);
      analysisManager->FillNtupleDColumn(14, weightE0);  //E0
      analysisManager->FillNtupleDColumn(15, weightRoseM1);
      analysisManager->FillNtupleDColumn(16, weightStieblingM1);
      
    }

  else if (fsboson ==0)

    //      +++++++++++++   background   ++++++++++++++++++++++++++++++
    {

      // // ------------------------    e+ e- sampled from k, wp, cth ----------------------------
      
      // first electron in cone around  y axis 
      //      G4double cthmax =1.;
      //      G4double cthmin = -1.;
      G4double cthmax =0.34; // 70-110 degrees
      G4double cthmin = -0.34;
      G4double costh1= cthmin + (cthmax - cthmin) * G4UniformRand();
      G4double phi1 =  2 * pi  * G4UniformRand();
      G4double sinth1 = sqrt( 1. - costh1 * costh1);
      // // cone around y axis
      // G4double py1 = costh1;
      // G4double px1 = sinth1*cos(phi1);
      // G4double pz1 = sinth1*sin(phi1);
      // // back to world coordinates
      //  costh1=pz1;
      //  phi1=atan2(py1,px1);

      G4double px1 = sinth1*cos(phi1);
      G4double py1 = sinth1*sin(phi1);
      G4double pz1 = costh1;

      // direction of normal to plane electron1-z
      G4ThreeVector p1 = G4ThreeVector(px1,py1,pz1);
      G4ThreeVector p1N =G4ThreeVector(py1,-px1,0);
      
      // // 2nd electron  generated at random 
      // cthmax = 1.0 ;
      // cthmin = -1.0;
      // G4double costh2 = cthmin + (cthmax - cthmin) * G4UniformRand();
      // G4double phi2 = 2 * pi * G4UniformRand() ;
      // G4double sinth2 = sqrt( 1. - costh2 * costh2);
      // G4double pz2 = costh2;
      // G4double px2 = sinth2 * cos(phi2);
      // G4double py2 = sinth2*sin(phi2);

      // opening angle generated at random around 1st electron
      G4double costh2r = -1 + 2.* G4UniformRand();
      //      G4double costh2r = -1 ;
      G4double phi2r = 2 * pi * G4UniformRand() ;
      G4double sinth2r = sqrt(1.-costh2r*costh2r);
      G4double px2r = sinth2r*cos(phi2r);
      G4double py2r = sinth2r*sin(phi2r);
      G4double pz2r = costh2r;
      G4ThreeVector p2r = G4ThreeVector(px2r,py2r,pz2r);
      G4ThreeVector p2 = p2r.rotate(acos(costh1),p1N);
      G4double px2 = p2.x();
      G4double py2 = p2.y();
      G4double pz2 = p2.z();
      G4double costh2  = pz2;
      G4double phi2 = atan2(py2,px2);

      G4double cosdth = px1*px2 + py1*py2 + pz1*pz2;  // cos of opening angle
      //      G4double cosdth = costh2r; 

      // G4cout << "px1 =" << px1 <<  "   py1 =" << py1 <<  "   pz1 =" << pz1 << G4endl;
      // G4cout << " costh2r = " << costh2r << "   phi2r = " << phi2r << G4endl; 
      // G4cout << "px2r =" << px2r <<  "   py2r =" << py2r <<  "   pz2r =" << pz2r << G4endl;
      // G4cout << "px2 =" << px2 <<  "   py2 =" << py2 <<  "   pz2 =" << pz2 << G4endl;
      // G4cout << " cosdth = " << cosdth << G4endl;

      // calculate the weight, based on  M.E. Rose, Phys. Rev. 76 (1949) 678
      
      // double akmean = 17.619/0.511;  // <===========
      // double aksigma = 0.0107/0.511;
      //      double akmean = 14.586/0.511;  // <===========
      //      double aksigma = 1.313/0.511;
      
      // double akmean = 18.34/0.511; // proton beam enwergy od 1225 keV
      double akmean = 19.7/0.511; // proton beam enwergy od 1225 keV
      double aksigma = 0.1/0.511; // 100 keV width ...
      double ak = G4RandGauss::shoot(akmean, aksigma);
      
      //           G4double ak = 6.05/0.511;  // in units of electron mass  // 6.129 for 3- level of 16O <<=====
      //      G4double akmax= 6.05/0.511;
      // if(ak<5)ak=5; // minimum 2.5 MeV
      
      G4double wp = 1. + (ak-2.) * G4UniformRand();      // choose wp randomly
      G4double wm = ak - wp;
      G4double pp = sqrt(wp*wp-1.);
      G4double pm = sqrt(wm*wm-1.);
      G4double pmz = pm * cosdth;
      G4double pmx = pm * sqrt(1.-cosdth*cosdth);
      G4double  q = sqrt((pp+pmz)*(pp+pmz) + pmx*pmx);
      G4double alpha = 1./137;
      
      //  Rose weight  E1 
      G4double t1 = 2*alpha/(2*pi) * (pp*pm/q) * (q/ak) / pow((ak*ak-q*q),2); // eq.(7) of Rose    
      G4double t2 = 3*(wp*wm+1-pp*pm*cosdth/3.);                                                                                                        
      G4double t3 = (q*q/(ak*ak)-2.) * (wp*wm-1.+pp*pm*cosdth);        
      G4double weightRoseE1 = t1*(t2+t3) ;   // arbitrary normalization ( d(cosdth), d(ak), d(wp) )
      
      //Stiebing weight, E1
      t1 = alpha/(16*pow(pi*ak,3))*pp*pm;
      t2 = 4*ak*ak/pow((ak*ak-q*q),2) + 2 * (pp*pp+pm*pm)/(ak*ak-q*q) +1;
      G4double   weightStieblingE1 = t1*t2 ;   // arbitrary normalization ( d(cosdth), d(ak), d(wp) )
      
      // E0  
      // G4double amass=sqrt( 2 * (1.+wp*wm-pp*pm*cosdth) );  // photon virtual mass
      // G4double alambda=amass;  // not sure 
      // //      G4double alambda=1;  // not sure 
      // G4double e2=alpha*4*pi;
      // G4double u=acosh(amass/2.);
      // G4double tanhu = tanh(u);
      // G4double tanh2u = tanh(2*u);
      // t1 = -4./3. * ( (1.+0.5/(cosh(u)*cosh(u))) * (1.-u*tanhu) -1./6. );
      // double t2t = 2.*(log(1./alambda)-1.) * (1.-2*u/tanh2u);
      // t2 = t2t + pi*pi/tanh2u - u/tanhu ;
      // t3 = -4./tanh2u* (  1./12 * (pi*pi + 6.*u*u + 12*u*log(1.-exp(-2*u)) -6.*PolyLog2(exp(-2*u))) ) ;
      // G4double t4 = -2*u/sinh(2*u);
      // G4double t5 =  (wp*wm-1.+pp*pm*cosdth+e2/pi*(wp-wm)*(wp-wm)*u/sinh(2*u)) * pp*pm;

      // G4double weightE0 = e2*e2/(9*pi)* (1.+4.*alpha*(t1+t2+t3+t4)*t5);
      //      G4cout << "wp = " << wp << ", dth = " << acos(cosdth) << ", weightE0 = " << weightE0 <<G4endl; 

      G4double e2=alpha*4*pi;
      G4double t0 = (4*e2*e2)/(27.*pi) * pow((ak/2.-1.),3) * pow((ak/2+1),2);
      G4double hk = (ak-2)/(ak+2);
      t1 = (6.+5.*(hk+1./hk)-2.*pow((hk+1./hk),2))/(10*hk) * Elliptic2(hk);
      t2 = ( (1.-hk*hk)*(hk*hk+5.*hk-3.) ) / (10.*hk*hk*hk) * Elliptic1(hk);
      t3 = (1. -hk*hk/5.) * cosdth;
      G4double weightE0 = t0 * ( t1 - t2 + t3);  // without radiative corrections

      // Rose M1
      t1 = 2*alpha/pi * (pp*pm/q) * pow((q/ak),3)  /    pow((ak*ak-q*q),2);
      t2 = 1.+ wp*wm -  pp*pm/(q*q) * (pm+pp*cosdth)*(pp+pm*cosdth);
      G4double   weightRoseM1 = t1*t2 ;   // arbitrary normalization ( d(cosdth), d(ak), d(wp) )
      
      //Stiebing weight, M1
      t1 = alpha/(16*pow(pi*ak,3))*pp*pm;
      t2 = 4*ak*ak/pow((ak*ak-q*q),2) + 2 * (pp*pp+pm*pm)/(ak*ak-q*q) -1;
      G4double   weightStieblingM1 = t1*t2 ;   // arbitrary normalization ( d(cosdth), d(ak), d(wp) )
      
      // // --------------------------------------------------------------------------------------
      
      fParticleGun->SetNumberOfParticles(nofParticles);  
      
      fParticleGun->SetParticleDefinition(particleDefinition);
      fParticleGun->SetParticleMomentumDirection(G4ThreeVector(px1,py1,costh1));
      //      G4cout << " positron k.e. = " << (wp-1.)*0.511 << G4endl;
      fParticleGun->SetParticleEnergy((wp-1.)*0.511);
      fParticleGun->SetParticlePosition(G4ThreeVector(0., 0., 0.*cm));
      //  fParticleGun->SetParticlePosition(G4ThreeVector(0.3*cm, 0., 0.*cm));  // <---------------
      fParticleGun->GeneratePrimaryVertex(anEvent);
      
      fParticleGun->SetParticleDefinition(particleDefinition2);
      fParticleGun->SetParticleMomentumDirection(G4ThreeVector(px2,py2,costh2));
      //      G4cout << " electron k.e. = " << (wm-1.)*0.511 << G4endl;
      fParticleGun->SetParticleEnergy((wm-1.)*0.511);
      fParticleGun->SetParticlePosition(G4ThreeVector(0., 0., 0.*cm));
      //  fParticleGun->SetParticlePosition(G4ThreeVector(0.3*cm, 0., 0.*cm));  // <---------------
      fParticleGun->GeneratePrimaryVertex(anEvent);
      
      // get analysis manager
      auto analysisManager = G4AnalysisManager::Instance();
      
      // fill histograms
      analysisManager->FillH1(1, costh1);
      analysisManager->FillH1(2, phi1);
      analysisManager->FillH1(3, costh2);
      analysisManager->FillH1(4, phi2);
      
      // fill ntuple
      analysisManager->FillNtupleDColumn(1, costh1);
      analysisManager->FillNtupleDColumn(2, phi1);
      analysisManager->FillNtupleDColumn(3, costh2);
      analysisManager->FillNtupleDColumn(4, phi2);
      analysisManager->FillNtupleDColumn(5, ak*0.511);
      analysisManager->FillNtupleDColumn(6, (wp-1.)*0.511);
      analysisManager->FillNtupleDColumn(7, (wm-1)*0.511);
      analysisManager->FillNtupleDColumn(8, acos(cosdth)*180./pi);
      G4double asymmetry=(wp-wm)/(wp+wm-2);
      analysisManager->FillNtupleDColumn(9, asymmetry);
      G4double ppx=pp*px1;
      G4double ppy = pp*py1;
      G4double ppz = pp*pz1;
      pmx=pm*px2;
      G4double pmy = pm*py2;
      pmz = pm*pz2;
      G4double m_ee = 0.511 * sqrt( (wp+wm)*(wp+wm) - (ppx+pmx) *(ppx+pmx)
				    - (ppy+pmy) *(ppy+pmy) - (ppz+pmz) *(ppz+pmz) );
      analysisManager->FillNtupleDColumn(10, m_ee);
      analysisManager->FillNtupleDColumn(13, weightRoseE1);
      //      analysisManager->FillNtupleDColumn(14, weightStieblingE1);
      analysisManager->FillNtupleDColumn(14, weightE0);
      analysisManager->FillNtupleDColumn(15, weightRoseM1);
      analysisManager->FillNtupleDColumn(16, weightStieblingM1);
    }
  else if (fsboson == -1)

    //      +++++++++++++   gamma   ++++++++++++++++++++++++++++++

    {  

      // Generate gamma, approximate energy distribution
      // G4double Eg = 18.15; // fixed  energy of gamma
      G4double Eg = 6.13 + (19.0-6.13)* G4UniformRand(); // uniform distribution between 6.13 and 19.0 MeV
      //      G4double EPCWeight = exp(log(10)*(-0.233 * Eg + 5.42));
      G4double EPCWeight = exp(-0.536 * Eg + 12.48);
      G4double EPCNorm =  -1./((19.0-6.13)*0.536)*( exp(-0.536*19.0+12.48) - exp(-0.536*6.13+12.48) ); // integral (sum of bins, not multiplied by bin width)
      //      G4cout << " -->> EPCNorm = " << EPCNorm << G4endl;
      EPCWeight= EPCWeight/EPCNorm;

      G4double costh_g = -1. + 2.*G4UniformRand(); // cos(theta) value of the  gamma [-1, 1]
      G4double sinth_g = sqrt(1.-costh_g*costh_g); // sin(theta) 
      G4double phi_g = 2*pi*G4UniformRand(); // Phi angle of the gamma [0, 2pi]

      G4double pxg = sinth_g*cos(phi_g);
      G4double pyg = sinth_g*sin(phi_g);
      G4double pzg = costh_g;
            
      // // --------------------------------------------------------------------------------------
      
      fParticleGun->SetNumberOfParticles(nofParticles);  
      
      fParticleGun->SetParticleDefinition(particleDefinition3);
      fParticleGun->SetParticleMomentumDirection(G4ThreeVector(pxg,pyg,pzg));
      fParticleGun->SetParticleEnergy((Eg));
      fParticleGun->SetParticlePosition(G4ThreeVector(0., 0., 0.*cm));
      fParticleGun->GeneratePrimaryVertex(anEvent);

      // get analysis manager
      auto analysisManager = G4AnalysisManager::Instance();
      
      // fill ntuple
      analysisManager->FillNtupleDColumn(38, costh_g);
      analysisManager->FillNtupleDColumn(39, phi_g);
      analysisManager->FillNtupleDColumn(40, Eg);
      analysisManager->FillNtupleDColumn(41, EPCWeight);
    }

  else if (fsboson == -2)


    //      +++++++++++++   cosmic Rays/ Fe-55 source   ++++++++++++++++++++++++++++++

    {  
      G4double Emu;
      fParticleGun->SetNumberOfParticles(nofParticles);        

      ////////////////////////////////////////////////////////////////////////////////////////////////////////////
      /// CR's
      //see: S. Tripathi & P. Shukla  https://onlinelibrary.wiley.com/doi/10.1155/ahep/8967757
      // and Gaisser formula: https://arxiv.org/abs/1509.06176,
      //    also https://www.sciencedirect.com/science/article/pii/S0927650501001438?ref=cra_js_challenge&fr=RR-1w
      // zenith angle distribution not affected by energy distribution
      // assume flux proportional to cos^2(theta)
      ////////////////////////////////////////////////////////////////////////////////////////////////////////////
      G4double costh_m;
      G4double EmuMin = 100*MeV;
      G4double EmuMax =20*GeV;
      G4double nvar=2.98, E0d=3.82*GeV, kvar=0.14*GeV; // parameters from https://onlinelibrary.wiley.com/doi/10.1155/ahep/8967757, Table 2
      //      G4double nvar=3.28, E0d=0.005*967*GeV, kvar=0.80*GeV;  Table 5
      // Table 2 parameters have better fit for Emu < 3 GeV
      
      // sample solid angle: cos(theta) and phi;  cos^2(theta) distribution,  theta = angle wrt azimuth
      G4int iok = -1;
      while (iok<0) {
	costh_m = 0.342 + (0.658)*G4UniformRand(); // cos(theta) value of the  muon, theta < 70 degrees
	G4double prob = costh_m * costh_m;  // range [0.117,1]
	if(0.117+G4UniformRand()*0.883 < prob){
	  //      costh_m = 1.; // to compare with Fig.2
	  //      costh_m = 0.259; // 75 degrees, to compare with Fig.2
	  // sample energy (eq. 4 of https://onlinelibrary.wiley.com/doi/10.1155/ahep/8967757
	  G4double A = E0d/costh_m;
	  G4double B = kvar/costh_m;
	  G4double Emu2 = (B + sqrt(B*B+4*A*B*nvar)/(2*nvar)); // Emu value for maximum flux
	  if(Emu2<EmuMin) Emu2=EmuMin;
	  if(Emu2>EmuMax) Emu2=EmuMax;
	  G4double probEmax = pow((E0d/costh_m +Emu2),-nvar) * exp(-kvar/(Emu2*costh_m));
	  // --- sample Emu linearly
	  Emu = EmuMin + (EmuMax - EmuMin)* G4UniformRand();
	  G4double prob = pow((E0d/costh_m +Emu),-nvar) * exp(-kvar/(Emu*costh_m));
	  if(G4UniformRand()*probEmax<prob){
	    iok=0;
	    break;
	  }
	}
      }
      	  
      G4double phi_m = 2*pi*G4UniformRand(); // Phi angle of the gamma [0, 2pi]
      G4double sinth_m = sqrt(1.-costh_m*costh_m); // sin(theta)

      // find max(probE) for the selected value of costh_m
      // G4double probEmax=0.;
      // G4double logEmuMin = log(EmuMin);
      // G4double logEmuMax = log(EmuMax);
      // for (int i=0; i<1000; i++)
      // 	{
      // 	  G4double logEmu = logEmuMin + (logEmuMax - logEmuMin)* G4UniformRand();
      // 	  Emu = exp(logEmu);
      // 	  G4double probE = pow((E0d/costh_m +Emu),-nvar) * exp(-kvar/(Emu*costh_m));
      // 	  if(probE>probEmax)probEmax=probE;
      // 	}

      fParticleGun->SetParticleDefinition(particleDefinition4); // mu+
							
      // // Initial position 
      G4double xinit = -10*cm + 20*cm*G4UniformRand();
      G4double yinit = 45*cm;
      G4double zinit = -15*cm + 30*cm*G4UniformRand();
      // Initial position at y = + 8 cm, x =0., z = 0.  <<===========
      // G4double xinit =3.*cm;
      // G4double yinit = 8.*cm;
      // G4double zinit = 0. ;
      // G4double xinit =0.*cm;
      // G4double yinit = 30.*cm;
      // G4double zinit = 0. ;
      

      G4double pxm = sinth_m*cos(phi_m);  // here theta is in direction of azimuth for CR's
      G4double pym = costh_m;
      G4double pzm = sinth_m*sin(phi_m);

      // //////////////
      // // Fe-55
      // //////////////
      // G4double rande = G4UniformRand();
      // if(rande < 0.105) {Emu = 5.6*keV;}
      // else{Emu = 2.7*keV;}
      // fParticleGun->SetParticleDefinition(particleDefinition3); // photon
      // costh_m = 1;
      // ////////////////////////////////
      
      // G4double phi_m = 2*pi*G4UniformRand(); // Phi angle of the gamma [0, 2pi]
      // G4double sinth_m = sqrt(1.-costh_m*costh_m); // sin(theta) 

      // G4double pxm = sinth_m*cos(phi_m); // here theta is in direction of azimuth for CR's
      // G4double pym = costh_m;
      // G4double pzm = sinth_m*sin(phi_m);

      // // Initial position at y = + 8 cm, x =0., z = 0.
      // //      G4double xinit = -0.4*cm+0.4*cm*G4UniformRand();
      // G4double xinit = 0.;
      // G4double yinit = 8.0*cm; 
      // G4double zinit = 0.*cm;
            
      // // --------------------------------------------------------------------------------------
      
      fParticleGun -> SetParticlePosition(G4ThreeVector(xinit, yinit, zinit));
      fParticleGun->SetParticleMomentumDirection(G4ThreeVector(-pxm,-pym,-pzm));
      fParticleGun->SetParticleEnergy((Emu));
      fParticleGun->GeneratePrimaryVertex(anEvent);

      // get analysis manager
      auto analysisManager = G4AnalysisManager::Instance();
      
      // fill ntuple
      analysisManager->FillNtupleDColumn(38, costh_m);
      analysisManager->FillNtupleDColumn(39, phi_m);
      analysisManager->FillNtupleDColumn(40, Emu);
      
    }

   else if (fsboson == -3)

    //      +++++++++++++   Sr90 source   ++++++++++++++++++++++++++++++

     {  
      // Generate e- 4-vectors from source.
      
      int iok = -1;
      G4double En = 0.;
      while (iok<0) {
	En = 2.5*G4UniformRand(); // Total kinetic energy of e- emission from 90Sr
	//	G4cout << "En = " << En << G4endl;
	G4int iEn = 1+100*En/2.5; //   between 1 and 101
	//	G4cout <<  "En = " << En << " IEn = " << iEn << G4endl;
	//	G4cout << iok << " " << iEn << G4endl;
	//	G4double prob = (*fiProb)[iEn];
	G4double prob = fiProb(iEn);
	//	G4cout << iok << " En = " << En << "  iEn = " << iEn << " prob = " << prob << G4endl;
	G4double newrand = G4UniformRand();
	// G4cout << " newrand = " << newrand << G4endl;
	if(prob > newrand){
	  iok =0;
	  //	  break;
	}
      }
      //      G4cout << En << G4endl;

      // --- muons CR
      // En = 4.*GeV;
      // ----------
      //-- electrons
      G4double TEn = En + 0.511; // total energy
      G4double pel  = sqrt(TEn*TEn-0.511*0.511); // Momentum of the e-

      //-- muons
      // G4double TEn = En + 105.; // total energy
      // G4double pel  = sqrt(TEn*TEn-105*105    ); // Momentum of the mu+

      //      G4double costh_e =  0.9998 + 0.0002*G4UniformRand();  // cos(theta) value of the  boson [1 degree)
      G4double costh_e =  -1.0 ;  // with respect to azymuth ( z points upward)
      G4double sinth_e = sqrt(1.-costh_e*costh_e); // sin(theta) value of the boson [-1, 1]
      G4double phi_e = 2*pi*G4UniformRand(); // Phi angle of the boson [0, 2pi]
      
      G4double pz_e = pel*sinth_e*cos(phi_e); // z component of momentum of e-
      G4double px_e = pel*sinth_e*cos(phi_e); // x component of momentum of e-
      G4double py_e = pel*costh_e; // y component of momentum of e-
      // G4cout << " p = " << px_e << "," << py_e << "," << pz_e << G4endl;
      G4double ux = px_e/pel;
      G4double uy = py_e/pel;
      G4double uz = pz_e/pel;
      // // --------------------------------------------------------------------------------------
      
      fParticleGun->SetNumberOfParticles(nofParticles);  
      
      fParticleGun->SetParticleDefinition(particleDefinition2);  // e-
      //      fParticleGun->SetParticleDefinition(particleDefinition4);  // mu+
      fParticleGun->SetParticleMomentumDirection(G4ThreeVector(ux,uy,uz));
      fParticleGun->SetParticleEnergy(En);
      G4double xin = -0.4*cm+0.4*cm*G4UniformRand();
      G4double yin = 8.0*cm  -0.4*cm+0.4*cm*G4UniformRand();
      fParticleGun->SetParticlePosition(G4ThreeVector(xin, yin, 0.*cm));
      fParticleGun->GeneratePrimaryVertex(anEvent);
      
      // get analysis manager
      auto analysisManager = G4AnalysisManager::Instance();
      
      // // fill histograms
      // analysisManager->FillH1(1, costh_p);
      // analysisManager->FillH1(2, phi_p);
      // analysisManager->FillH1(3, costh_m);
      // analysisManager->FillH1(4, phi_m);
      
      // fill ntuple
      analysisManager->FillNtupleDColumn(38, costh_e);
      analysisManager->FillNtupleDColumn(39, phi_e);
      analysisManager->FillNtupleDColumn(40, En);
    }   

}
  


//....oooOO0OOooo........oooOO0OOooo........oooOO0OOooo........oooOO0OOooo......
