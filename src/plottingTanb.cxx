#include <map>
#include <vector>
#include "TH1F.h"
#include "TColor.h"
#include "TGraph.h"
#include "TString.h"
#include "TCanvas.h"
#include "TLegend.h"
#include "TPaveText.h"
#include "TGraphAsymmErrors.h"
#include "TROOT.h"
#include "TLine.h"
#include "TH2D.h"
//#include "TRint.h"

#include <iostream>

void
plottingTanb(TCanvas& canv, TH2D* h2d, std::vector<TGraph*> minus2sigma, std::vector<TGraph*> minus1sigma, std::vector<TGraph*> expected, std::vector<TGraph*> plus1sigma, std::vector<TGraph*> plus2sigma, std::vector<TGraph*> observed, std::vector<TGraph*> injected, std::vector<std::vector<TGraph*>> higgsBands, std::map<std::string, TGraph*> comparisons, std::string& xaxis, std::string& yaxis, std::string& theory, double min=0., double max=50., bool log=false, bool transparent=false, bool expectedOnly=false, bool MSSMvsSM=true, std::string HIG="", bool Brazilian=false, bool azh=false)
{
  // set up styles
  canv.cd();
  if(log){ 
    canv.SetLogy(1); 
   //  canv.SetLogx(1); 
  }
  canv.SetTopMargin(0.25);
  // setup the CMS colors
  TColor* obs = new TColor(1501, 0.463, 0.867, 0.957);
  if(transparent && !Brazilian) obs->SetAlpha(0.5); 
  TColor* twosigma = gROOT->GetColor(kGray+1);
  //if(transparent) twosigma->SetAlpha(0.5);
  TColor* onesigma = gROOT->GetColor(kGray+2);
  //if(transparent) onesigma->SetAlpha(0.5);
  TColor* ph = gROOT->GetColor(kYellow);
  ph->SetAlpha(0.0);
  TColor* backgroundColor = gROOT->GetColor(kRed);
  //backgroundColor->SetAlpha(0.2);

  h2d->GetXaxis()->SetTitle(xaxis.c_str()); 
  //h2d->GetXaxis()->SetRange(expected[0]->GetX()[0]-shift_label*.01, expected[0]->GetX()[expected[0]->GetN()-1]+.01);
  h2d->GetXaxis()->SetRangeUser(h2d->GetXaxis()->GetBinLowEdge(1), h2d->GetXaxis()->GetBinLowEdge(h2d->GetNbinsX()-2)+h2d->GetXaxis()->GetBinWidth(h2d->GetNbinsX()-2));
  h2d->GetXaxis()->SetLabelFont(62);
  h2d->GetXaxis()->SetTitleColor(1);
  h2d->GetXaxis()->SetTitleOffset(1.05);
  h2d->GetXaxis()->SetTitleFont(62);
  h2d->GetYaxis()->SetTitle(yaxis.c_str()); 
  h2d->GetYaxis()->SetRangeUser(min, max);
  h2d->GetYaxis()->SetLabelFont(62);
  h2d->GetYaxis()->SetTitleSize(0.05);
  h2d->GetYaxis()->SetTitleOffset(1.30); //1.12
  h2d->GetYaxis()->SetLabelSize(0.05);
  h2d->GetZaxis()->SetTitle("CL_{S}/0.05");
  h2d->GetZaxis()->SetLabelFont(62);
  h2d->GetZaxis()->SetTitleSize(0.04);
  h2d->GetZaxis()->SetTitleOffset(1.03);
  h2d->GetZaxis()->SetLabelSize(0.04);
//   h2d->SetMinimum(0.);
  h2d->SetNameTitle("","");
  h2d->Draw("AXIS");

  //mass constraint (background color)
  TGraphAsymmErrors* background = new TGraphAsymmErrors();
  background->SetFillStyle(3005.); //1001
  background->SetFillColor(backgroundColor->GetNumber());
  background->SetLineWidth(2);
  background->SetLineColor(kWhite);  

  TGraphAsymmErrors* mHconstraint = new TGraphAsymmErrors();
  mHconstraint->SetFillStyle(1001.); //1001
  mHconstraint->SetFillColor(kWhite);
  mHconstraint->SetLineWidth(2);
  mHconstraint->SetLineStyle(3);
  mHconstraint->SetLineColor(kGreen+3);  

  TGraphAsymmErrors* emptybackground = new TGraphAsymmErrors();
  emptybackground->SetFillStyle(1001.);
  emptybackground->SetFillColor(kWhite);
  emptybackground->SetLineColor(kWhite);
  
  
  //exclusion graphs
  for(unsigned int i=0; i<minus2sigma.size(); i++){
    minus2sigma[i]->SetFillStyle(1001);
    minus2sigma[i]->SetFillColor(twosigma->GetNumber()); 
    minus2sigma[i]->SetLineColor(kWhite);
    if(Brazilian) minus2sigma[i]->SetFillColor(TColor::GetColor(252,241,15));
    minus2sigma[i]->SetLineWidth(1);
    minus2sigma[i]->Draw("F SAME");    
  }
  for(unsigned int i=0; i<minus1sigma.size(); i++){
    minus1sigma[i]->SetFillStyle(1001);
    minus1sigma[i]->SetFillColor(onesigma->GetNumber()); 
    minus1sigma[i]->SetLineColor(kWhite); 
    if(Brazilian) minus1sigma[i]->SetFillColor(kGreen);
    minus1sigma[i]->SetLineWidth(1);
    minus1sigma[i]->Draw("F SAME");  
  }
  for(unsigned int i=0; i<plus1sigma.size(); i++){
    plus1sigma[i]->SetFillStyle(1001);
    plus1sigma[i]->SetFillColor(twosigma->GetNumber()); 
    plus1sigma[i]->SetLineColor(kWhite);
    if(Brazilian) plus1sigma[i]->SetFillColor(TColor::GetColor(252,241,15));
    plus1sigma[i]->SetLineWidth(1);
    plus1sigma[i]->Draw("F SAME"); 
  }
  for(unsigned int i=0; i<plus2sigma.size(); i++){
    plus2sigma[i]->SetFillStyle(1001);
    plus2sigma[i]->SetFillColor(kWhite); 
    plus2sigma[i]->SetLineColor(kWhite);
    plus2sigma[i]->SetLineWidth(1);
    plus2sigma[i]->Draw("F SAME"); 
  }
      
  for(unsigned int i=0; i<expected.size(); i++){
    expected[i]->SetLineColor(kBlack); 
    expected[i]->SetLineWidth(2);
    expected[i]->SetLineStyle(2); 
    expected[i]->Draw("L SAME"); //CONT
  }  

  if(injected[0]){
    for(unsigned int i=0; i<injected.size(); i++){
      injected[i]->SetLineColor(kBlue);
      injected[i]->SetLineWidth(2);
      injected[i]->SetLineStyle(1);
      injected[i]->Draw("Lsame");
    }
  }

  if(!expectedOnly){
    for(unsigned int i=0; i<observed.size(); i++){
      observed[i]->SetFillColor(obs->GetNumber());
      observed[i]->SetFillStyle(1001);
      observed[i]->SetMarkerStyle(20);
      observed[i]->SetMarkerSize(1.0);
      observed[i]->SetMarkerColor(kBlack);
      //observed[i]->SetLineWidth(3.); 
      observed[i]->SetLineColor(kBlack); 
      if(Brazilian) observed[i]->SetFillColor(kBlack); 
      if(Brazilian) observed[i]->SetFillStyle(3004);  
      observed[i]->Draw("F SAME"); 
      observed[i]->Draw("L SAME");  //CONT
    } 
  }

  //Constraint for Hhh
  if(theory!="2HDM type-I" && theory!="2HDM type-II"){
    if(!azh && higgsBands[2][0]!=0 && higgsBands[3][0]!=0){
      for(unsigned int i=0; i<higgsBands[2].size(); i++){
	higgsBands[2][i]->SetLineWidth(9902);
	higgsBands[2][i]->SetFillStyle(1001);
	higgsBands[2][i]->SetFillColor(kWhite); 
	higgsBands[2][i]->SetLineColor(kGreen+3);
	higgsBands[2][i]->SetLineStyle(3);
	higgsBands[2][i]->Draw("L SAME"); 
      }
      for(unsigned int i=0; i<higgsBands[3].size(); i++){
	higgsBands[3][i]->SetLineWidth(-9902);
	higgsBands[3][i]->SetFillStyle(1001);
	higgsBands[3][i]->SetFillColor(kWhite);
	higgsBands[3][i]->SetLineColor(kGreen+3);
	higgsBands[3][i]->SetLineStyle(3);
	higgsBands[3][i]->Draw("L SAME");
      }
    }
    //Mass constraint
    for(unsigned int i=0; i<higgsBands[0].size(); i++){
      higgsBands[0][i]->SetLineWidth(402);
      higgsBands[0][i]->SetFillStyle(3005);
      higgsBands[0][i]->SetFillColor(backgroundColor->GetNumber()); 
      higgsBands[0][i]->SetLineColor(backgroundColor->GetNumber());
      higgsBands[0][i]->Draw("L SAME"); 
    }
    for(unsigned int i=0; i<higgsBands[1].size(); i++){
      higgsBands[1][i]->SetLineWidth(-402);
      higgsBands[1][i]->SetFillStyle(3005);
      higgsBands[1][i]->SetFillColor(backgroundColor->GetNumber()); 
      higgsBands[1][i]->SetLineColor(backgroundColor->GetNumber());
      higgsBands[1][i]->Draw("L SAME");
    }
 }

  int idx=0;
  std::map<std::string,int> coloredComps;
  coloredComps["arXiv_1211_6956" ] = kOrange+3;
  coloredComps["arXiv_1204_2760" ] = kGreen+4;
  coloredComps["arXiv_1302_2892" ] = kBlue;
  coloredComps["arXiv_1205_5736" ] = kRed;
  coloredComps["HIG_12_052_lower"] = kRed;
  coloredComps["HIG_12_052_upper"] = kRed;
  for(std::map<std::string,TGraph*>::const_iterator comp = comparisons.begin(); comp!=comparisons.end(); ++comp, ++idx){
    comp->second->SetLineColor(coloredComps[comp->second->GetName()]);
    comp->second->SetFillColor(coloredComps[comp->second->GetName()]);
    if(std::string(comp->second->GetName())==std::string("HIG_12_052_upper")){
      comp->second->SetFillStyle(3004);
      comp->second->SetLineWidth(-702);
    }
    else{
      comp->second->SetFillStyle(3005);
      comp->second->SetLineWidth(+702);
    }
    comp->second->Draw("same");
  }
  
  TPaveText* theory1;
  theory1= new TPaveText(0.24, 0.95, 0.76, 0.985, "NDC");
  theory1->SetBorderSize(   0 );
  theory1->SetFillStyle(    0 );
  theory1->SetTextAlign(   12 );
  theory1->SetTextSize ( 0.04);
  theory1->SetTextColor(    1 );
  theory1->SetTextFont (   62 );
  theory1->AddText(theory.c_str());

  /// add the proper legend
  TLegend* leg = new TLegend(0.08, 0.79, 0.61, 0.945);
  leg->SetNColumns(2);
  leg->SetBorderSize(  0 );
  leg->SetFillStyle (1001);
  leg->SetTextSize  (0.03);
  leg->SetTextFont  ( 62 ); 
  leg->SetFillColor (kWhite);
  leg->SetLineColor (kBlack);
  if(MSSMvsSM) leg->SetHeader("CL_{S}(MSSM,SM)<0.05:");
  else leg->SetHeader("95% CL Excluded:");
  if(!expectedOnly){ 
    if(!Brazilian) {
      if(observed[0]) observed[0]->SetFillColor(obs->GetNumber());
    }
    if(observed[0]) leg->AddEntry(observed[0], "Observed", "FL");
  }
  else leg->AddEntry(emptybackground,"","F");
  if(minus1sigma[0]) leg->AddEntry(minus1sigma[0], "#pm 1#sigma Expected","F");
  if(expected[0]) leg->AddEntry(expected[0], "Expected", "L");
  if(minus2sigma[0]) leg->AddEntry(minus2sigma[0], "#pm 2#sigma Expected", "F"); 
  if(injected[0]) leg->AddEntry(injected[0], "h_{SM} injected", "L");
  for(std::map<std::string,TGraph*>::const_iterator comp = comparisons.begin(); comp!=comparisons.end(); ++comp){
    if(std::string(comp->first) == std::string("EMPTY")) { continue; }
    else if(std::string(comp->first) == std::string("HIG-12-050 exp")) {
      leg->AddEntry(comp->second, (comp->first).c_str(), "L");
    }
    else{
      leg->AddEntry(comp->second, (comp->first).c_str(), "FL");
    }
  }
  leg->Draw("same");

  TLegend* leg2 = new TLegend(0.60, 0.79, 0.95, 0.945);
  leg2->SetBorderSize( 0  );
  leg2->SetFillStyle (1001);
  leg2->SetTextSize  (0.03);
  leg2->SetTextFont  ( 62 ); 
  leg2->SetFillColor (kWhite);
  leg2->SetLineWidth (2);
  leg2->SetLineColor (kBlack);
  leg2->SetHeader("Mass constraint:");
  if(log) {
    if(theory=="MSSM low-m_{H} scenario") leg2->AddEntry(background, "m^{MSSM}_{H} #scale[1.7]{#bf{#neq}} 125#pm3 GeV", "F");
    else leg2->AddEntry(background, "m^{MSSM}_{h} #scale[1.7]{#bf{#neq}} 125#pm3 GeV", "F");
    if(theory=="MSSM low-tan#beta-high scenario"&&!azh) leg2->AddEntry(mHconstraint," m^{MSSM}_{H} #notin [260,350] GeV","F");
    else leg2->AddEntry(emptybackground,"","F");
  }
  else {
    if(theory=="MSSM low-m_{H} scenario") leg2->AddEntry(background, "m^{MSSM}_{H} #scale[1.7]{#bf{#neq}} 125#pm3 GeV", "F");
    else leg2->AddEntry(background, "m^{MSSM}_{h} #scale[1.7]{#bf{ #neq}} 125#pm3 GeV", "FL");
    if(theory=="MSSM low-tan#beta-high scenario"&&!azh) leg2->AddEntry(mHconstraint," m^{MSSM}_{H} #notin [260,350] GeV","F");
    else leg2->AddEntry(emptybackground,"","F");
  }
  if(theory!="2HDM type-I" && theory!="2HDM type-II") leg2->Draw("same");

  TLine* legline;
  legline = new TLine(605, 13, 680, 13);
  legline->SetLineWidth(3);
  legline->SetLineColor(backgroundColor->GetNumber());
  //if(theory!="2HDM type-I" && theory!="2HDM type-II") legline->DrawLineNDC(leg2->GetX1()+0.015, leg2->GetY2()-0.01, leg2->GetX1()+0.072, leg2->GetY2()-0.01);
  if(theory!="2HDM type-I" && theory!="2HDM type-II") legline->DrawLineNDC(leg2->GetX1()+0.015, leg2->GetY2()-0.055, leg2->GetX1()+0.072, leg2->GetY2()-0.055);

  theory1->Draw("same");

  canv.RedrawAxis();
  return;
}
