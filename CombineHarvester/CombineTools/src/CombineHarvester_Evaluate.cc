#include "CombineTools/interface/CombineHarvester.h"
#include <vector>
#include <map>
#include <string>
#include <iostream>
#include <utility>
#include <set>
#include <fstream>
#include "boost/lexical_cast.hpp"
#include "boost/algorithm/string.hpp"
#include "boost/range/algorithm_ext/erase.hpp"
#include "boost/range/algorithm/find.hpp"
#include "boost/format.hpp"
#include "TDirectory.h"
#include "TH1.h"
#include "CombineTools/interface/Observation.h"
#include "CombineTools/interface/Process.h"
#include "CombineTools/interface/Systematic.h"
#include "CombineTools/interface/Parameter.h"
#include "CombineTools/interface/MakeUnique.h"
#include "CombineTools/interface/Utilities.h"
#include "CombineTools/interface/Algorithm.h"

// #include "TMath.h"
// #include "boost/format.hpp"
// #include "Utilities/interface/FnPredicates.h"
// #include "Math/QuantFuncMathCore.h"

namespace ch {

CombineHarvester::ProcSystMap CombineHarvester::GenerateProcSystMap() {
  ProcSystMap lookup(procs_.size());
  for (unsigned i = 0; i < systs_.size(); ++i) {
    for (unsigned j = 0; j < procs_.size(); ++j) {
      if (MatchingProcess(*(systs_[i]), *(procs_[j]))) {
        lookup[j].push_back(systs_[i].get());
      }
    }
  }
  return lookup;
}

double CombineHarvester::GetUncertainty() {
  auto lookup = GenerateProcSystMap();
  double err_sq = 0.0;
  for (auto param_it : params_) {
    double backup = param_it.second->val();
    param_it.second->set_val(backup+param_it.second->err_d());
    double rate_d = this->GetRateInternal(lookup, param_it.first);
    param_it.second->set_val(backup+param_it.second->err_u());
    double rate_u = this->GetRateInternal(lookup, param_it.first);
    double err = std::fabs(rate_u-rate_d) / 2.0;
    err_sq += err * err;
    param_it.second->set_val(backup);
  }
  return std::sqrt(err_sq);
}

double CombineHarvester::GetUncertainty(RooFitResult const* fit,
                                        unsigned n_samples) {
  auto lookup = GenerateProcSystMap();
  double rate = GetRateInternal(lookup);
  double err_sq = 0.0;

  // Create a backup copy of the current parameter values
  auto backup = GetParameters();

  // Calling randomizePars() ensures that the RooArgList of sampled parameters
  // is already created within the RooFitResult
  RooArgList const& rands = fit->randomizePars();

  // Now create two aligned vectors of the RooRealVar parameters and the
  // corresponding ch::Parameter pointers
  int n_pars = rands.getSize();
  std::vector<RooRealVar const*> r_vec(n_pars, nullptr);
  std::vector<ch::Parameter*> p_vec(n_pars, nullptr);
  for (unsigned n = 0; n < p_vec.size(); ++n) {
    r_vec[n] = dynamic_cast<RooRealVar const*>(rands.at(n));
    p_vec[n] = GetParameter(r_vec[n]->GetName());
  }

  for (unsigned i = 0; i < n_samples; ++i) {
    // Randomise and update values
    fit->randomizePars();
    for (int n = 0; n < n_pars; ++n) {
      if (p_vec[n]) p_vec[n]->set_val(r_vec[n]->getVal());
    }

    double rand_rate = this->GetRateInternal(lookup);
    double err = std::fabs(rand_rate-rate);
    err_sq += (err*err);
  }
  this->UpdateParameters(backup);
  return std::sqrt(err_sq/double(n_samples));
}

TH1F CombineHarvester::GetShapeWithUncertainty() {
  auto lookup = GenerateProcSystMap();
  TH1F shape = GetShape();
  for (int i = 1; i <= shape.GetNbinsX(); ++i) {
    shape.SetBinError(i, 0.0);
  }
  for (auto param_it : params_) {
    double backup = param_it.second->val();
    param_it.second->set_val(backup+param_it.second->err_d());
    TH1F shape_d = this->GetShapeInternal(lookup, param_it.first);
    param_it.second->set_val(backup+param_it.second->err_u());
    TH1F shape_u = this->GetShapeInternal(lookup, param_it.first);
    for (int i = 1; i <= shape.GetNbinsX(); ++i) {
      double err =
          std::fabs(shape_u.GetBinContent(i) - shape_d.GetBinContent(i)) / 2.0;
      shape.SetBinError(i, err*err + shape.GetBinError(i));
    }
    param_it.second->set_val(backup);
  }
  for (int i = 1; i <= shape.GetNbinsX(); ++i) {
    shape.SetBinError(i, std::sqrt(shape.GetBinError(i)));
  }
  return shape;
}

TH1F CombineHarvester::GetShapeWithUncertainty(RooFitResult const* fit,
                                               unsigned n_samples) {
  auto lookup = GenerateProcSystMap();
  TH1F shape = GetShapeInternal(lookup);
  for (int i = 1; i <= shape.GetNbinsX(); ++i) {
    shape.SetBinError(i, 0.0);
  }
  // Create a backup copy of the current parameter values
  auto backup = GetParameters();

  // Calling randomizePars() ensures that the RooArgList of sampled parameters
  // is already created within the RooFitResult
  RooArgList const& rands = fit->randomizePars();

  // Now create two aligned vectors of the RooRealVar parameters and the
  // corresponding ch::Parameter pointers
  int n_pars = rands.getSize();
  std::vector<RooRealVar const*> r_vec(n_pars, nullptr);
  std::vector<ch::Parameter*> p_vec(n_pars, nullptr);
  for (unsigned n = 0; n < p_vec.size(); ++n) {
    r_vec[n] = dynamic_cast<RooRealVar const*>(rands.at(n));
    p_vec[n] = GetParameter(r_vec[n]->GetName());
  }

  // Main loop through n_samples
  for (unsigned i = 0; i < n_samples; ++i) {
    // Randomise and update values
    fit->randomizePars();
    for (int n = 0; n < n_pars; ++n) {
      if (p_vec[n]) p_vec[n]->set_val(r_vec[n]->getVal());
    }

    TH1F rand_shape = this->GetShapeInternal(lookup);
    for (int i = 1; i <= shape.GetNbinsX(); ++i) {
      double err =
          std::fabs(rand_shape.GetBinContent(i) - shape.GetBinContent(i));
      shape.SetBinError(i, err*err + shape.GetBinError(i));
    }
  }
  for (int i = 1; i <= shape.GetNbinsX(); ++i) {
    shape.SetBinError(i, std::sqrt(shape.GetBinError(i)/double(n_samples)));
  }
  this->UpdateParameters(backup);
  return shape;
}

double CombineHarvester::GetRate() {
  auto lookup = GenerateProcSystMap();
  return GetRateInternal(lookup);
}

TH1F CombineHarvester::GetShape() {
  auto lookup = GenerateProcSystMap();
  return GetShapeInternal(lookup);
}

double CombineHarvester::GetRateInternal(ProcSystMap const& lookup,
    std::string const& single_sys) {
  double rate = 0.0;
  // TH1F tot_shape
  for (unsigned i = 0; i < procs_.size(); ++i) {
    double p_rate = procs_[i]->rate();
    // If we are evaluating the effect of a single parameter
    // check the list of associated nuisances and skip if
    // this "single_sys" is not in the list
    // However - we can't skip if the process has a pdf, as
    // we haven't checked what the parameters are
    if (single_sys != "" && !procs_[i]->pdf()) {
      if (!ch::any_of(lookup[i], [&](Systematic const* sys) {
        return sys->name() == single_sys;
      })) continue;
    }
    for (auto sys_it : lookup[i]) {
      double x = params_[sys_it->name()]->val();
      if (sys_it->asymm()) {
        if (x >= 0) {
          p_rate *= std::pow(sys_it->value_u(), x * sys_it->scale());
        } else {
          p_rate *= std::pow(sys_it->value_d(), -1.0 * x * sys_it->scale());
        }
      } else {
        p_rate *= std::pow(sys_it->value_u(), x * sys_it->scale());
      }
    }
    rate += p_rate;
  }
  return rate;
}

TH1F CombineHarvester::GetShapeInternal(ProcSystMap const& lookup,
    std::string const& single_sys) {
  TH1F shape;
  bool shape_init = false;

  for (unsigned i = 0; i < procs_.size(); ++i) {
    // Might be able to skip if only interested in one nuisance
    // However - we can't skip if the process has a pdf, as
    // we haven't checked what the parameters are
    if (single_sys != "" && !procs_[i]->pdf()) {
      if (!ch::any_of(lookup[i], [&](Systematic const* sys) {
        return sys->name() == single_sys;
      })) continue;
    }

    double p_rate = procs_[i]->rate();
    if (procs_[i]->shape() || procs_[i]->data()) {
      TH1F proc_shape = procs_[i]->ShapeAsTH1F();
      for (auto sys_it : lookup[i]) {
        double x = params_[sys_it->name()]->val();
        if (sys_it->asymm()) {
          if (x >= 0) {
            p_rate *= std::pow(sys_it->value_u(), x * sys_it->scale());
          } else {
            p_rate *= std::pow(sys_it->value_d(), -1.0 * x * sys_it->scale());
          }
          if (sys_it->type() == "shape") {
            if (sys_it->shape_u() && sys_it->shape_d()) {
              ShapeDiff(x * sys_it->scale(), &proc_shape, procs_[i]->shape(),
                        sys_it->shape_d(), sys_it->shape_u());
            }
            if (sys_it->data_u() && sys_it->data_d()) {
              RooDataHist const* nom =
                  dynamic_cast<RooDataHist const*>(procs_[i]->data());
              if (nom) {
                ShapeDiff(x * sys_it->scale(), &proc_shape, nom,
                          sys_it->data_d(), sys_it->data_u());
              }
            }
          }
        } else {
          p_rate *= std::pow(sys_it->value_u(), x * sys_it->scale());
        }
      }
      for (int b = 1; b <= proc_shape.GetNbinsX(); ++b) {
        if (proc_shape.GetBinContent(b) < 0.) proc_shape.SetBinContent(b, 0.);
      }
      proc_shape.Scale(p_rate);
      if (!shape_init) {
        proc_shape.Copy(shape);
        shape.Reset();
        shape_init = true;
      }
      shape.Add(&proc_shape);
    } else if (procs_[i]->pdf()) {
      RooAbsData const* data_obj = FindMatchingData(procs_[i].get());
      std::string var_name = "CMS_th1x";
      if (data_obj) var_name = data_obj->get()->first()->GetName();
      TH1::AddDirectory(false);
      TH1F proc_shape = *(dynamic_cast<TH1F*>(
          procs_[i]->pdf()->createHistogram(var_name.c_str())));
      if (!procs_[i]->pdf()->selfNormalized()) {
        // LOGLINE(log(), "Have a pdf that is not selfNormalized");
        // std::cout << "Integral: " << proc_shape.Integral() << "\n";
        if (proc_shape.Integral() > 0.) {
          proc_shape.Scale(1. / proc_shape.Integral());
        }
      }
      for (auto sys_it : lookup[i]) {
        double x = params_[sys_it->name()]->val();
        if (sys_it->asymm()) {
          if (x >= 0) {
            p_rate *= std::pow(sys_it->value_u(), x * sys_it->scale());
          } else {
            p_rate *= std::pow(sys_it->value_d(), -1.0 * x * sys_it->scale());
          }
        } else {
          p_rate *= std::pow(sys_it->value_u(), x * sys_it->scale());
        }
      }
      proc_shape.Scale(p_rate);
      if (!shape_init) {
        proc_shape.Copy(shape);
        shape.Reset();
        shape_init = true;
      }
      shape.Add(&proc_shape);
    }
  }
  return shape;
}

double CombineHarvester::GetObservedRate() {
  double rate = 0.0;
  for (unsigned i = 0; i < obs_.size(); ++i) {
    rate += obs_[i]->rate();
  }
  return rate;
}

TH1F CombineHarvester::GetObservedShape() {
  TH1F shape;
  bool shape_init = false;

  for (unsigned i = 0; i < obs_.size(); ++i) {
    TH1F proc_shape;
    double p_rate = obs_[i]->rate();
    if (obs_[i]->shape()) {
      proc_shape = obs_[i]->ShapeAsTH1F();
    } else if (obs_[i]->data()) {
      std::string var_name = obs_[i]->data()->get()->first()->GetName();
      proc_shape = *(dynamic_cast<TH1F*>(obs_[i]->data()->createHistogram(
                             var_name.c_str())));
      proc_shape.Scale(1. / proc_shape.Integral());
    }
    proc_shape.Scale(p_rate);
    if (!shape_init) {
      proc_shape.Copy(shape);
      shape.Reset();
      shape_init = true;
    }
    shape.Add(&proc_shape);
  }
  return shape;
}

void CombineHarvester::ShapeDiff(double x,
    TH1F * target,
    TH1 const* nom,
    TH1 const* low,
    TH1 const* high) {
  double fx = smoothStepFunc(x);
  for (int i = 1; i <= target->GetNbinsX(); ++i) {
    float h = high->GetBinContent(i);
    float l = low->GetBinContent(i);
    float n = nom->GetBinContent(i);
    target->SetBinContent(i, target->GetBinContent(i) +
                                 0.5 * x * ((h - l) + (h + l - 2. * n) * fx));
  }
}

void CombineHarvester::ShapeDiff(double x,
    TH1F * target,
    RooDataHist const* nom,
    RooDataHist const* low,
    RooDataHist const* high) {
  double fx = smoothStepFunc(x);
  for (int i = 1; i <= target->GetNbinsX(); ++i) {
    high->get(i-1);
    low->get(i-1);
    nom->get(i-1);
    // The RooDataHists are not scaled to unity (unlike in the TH1 case above)
    // so we have to normalise the bin contents on the fly
    float h = high->weight() / high->sumEntries();
    float l = low->weight() / low->sumEntries();
    float n = nom->weight() / nom->sumEntries();
    target->SetBinContent(i, target->GetBinContent(i) +
                                 0.5 * x * ((h - l) + (h + l - 2. * n) * fx));
  }
}

// void CombineHarvester::SetParameters(std::vector<ch::Parameter> params) {
//   params_.clear();
//   for (unsigned i = 0; i < params.size(); ++i) {
//     params_[params[i].name()] = std::make_shared<ch::Parameter>(params[i]);
//   }
// }

void CombineHarvester::RenameParameter(std::string const& oldname,
                                       std::string const& newname) {
  auto it = params_.find(oldname);
  if (it != params_.end()) {
    params_[newname] = it->second;
    params_[newname]->set_name(newname);
    params_.erase(it);
  }
}

ch::Parameter const* CombineHarvester::GetParameter(
    std::string const& name) const {
  auto it = params_.find(name);
  if (it != params_.end()) {
    return it->second.get();
  } else {
    return nullptr;
  }
}

ch::Parameter* CombineHarvester::GetParameter(std::string const& name) {
  auto it = params_.find(name);
  if (it != params_.end()) {
    return it->second.get();
  } else {
    return nullptr;
  }
}

void CombineHarvester::UpdateParameters(
    std::vector<ch::Parameter> const& params) {
  for (unsigned i = 0; i < params.size(); ++i) {
    auto it = params_.find(params[i].name());
    if (it != params_.end()) {
      it->second->set_val(params[i].val());
      it->second->set_err_d(params[i].err_d());
      it->second->set_err_u(params[i].err_u());
    } else {
      if (verbosity_ >= 1) {
        LOGLINE(log(), "Parameter " + params[i].name() + " is not defined");
      }
    }
  }
}

void CombineHarvester::UpdateParameters(RooFitResult const* fit) {
  // check for fit == null here
  for (int i = 0; i < fit->floatParsFinal().getSize(); ++i) {
    RooRealVar const* var =
        dynamic_cast<RooRealVar const*>(fit->floatParsFinal().at(i));
    // check for failed cast here
    auto it = params_.find(std::string(var->GetName()));
    if (it != params_.end()) {
      it->second->set_val(var->getVal());
      it->second->set_err_d(var->getErrorLo());
      it->second->set_err_u(var->getErrorHi());
    } else {
      if (verbosity_ >= 1) {
        LOGLINE(log(),
                "Parameter " + std::string(var->GetName()) + " is not defined");
      }
    }
  }
}

std::vector<ch::Parameter> CombineHarvester::GetParameters() const {
  std::vector<ch::Parameter> params;
  for (auto const& it : params_) {
    params.push_back(*(it.second));
  }
  return params;
}

void CombineHarvester::VariableRebin(std::vector<double> bins) {
  // We need to keep a record of the Process rates before we rebin. The
  // reasoning comes from the following scenario: the user might choose a new
  // binning which excludes some of the existing bins - thus changing the
  // process normalisation. This is fine, but we also need to adjust the shape
  // Systematic entries - both the rebinning and the adjustment of the value_u
  // and value_d shifts.
  std::vector<double> prev_proc_rates(procs_.size());

  // Also hold on the scaled Process hists *after* the rebinning is done - these
  // are needed to update the associated Systematic entries
  std::vector<std::unique_ptr<TH1>> scaled_procs(procs_.size());

  for (unsigned i = 0; i < procs_.size(); ++i) {
    if (procs_[i]->shape()) {
      // Get the scaled shape here
      std::unique_ptr<TH1> copy(procs_[i]->ClonedScaledShape());
      // shape norm should only be "no_norm_rate"
      prev_proc_rates[i] = procs_[i]->no_norm_rate();
      std::unique_ptr<TH1> copy2(copy->Rebin(bins.size()-1, "", &(bins[0])));
      // The process shape & rate will be reset here
      procs_[i]->set_shape(std::move(copy2), true);
      scaled_procs[i] = procs_[i]->ClonedScaledShape();
    }
  }
  for (unsigned i = 0; i < obs_.size(); ++i) {
    if (obs_[i]->shape()) {
      std::unique_ptr<TH1> copy(obs_[i]->ClonedScaledShape());
      std::unique_ptr<TH1> copy2(copy->Rebin(bins.size()-1, "", &(bins[0])));
      obs_[i]->set_shape(std::move(copy2), true);
    }
  }
  for (unsigned i = 0; i < systs_.size(); ++i) {
    TH1 const* proc_hist = nullptr;
    double prev_rate = 0.;
    for (unsigned j = 0; j < procs_.size(); ++j) {
      if (MatchingProcess(*(procs_[j]), *(systs_[i].get()))) {
        proc_hist = scaled_procs[j].get();
        prev_rate = prev_proc_rates[j];
      }
    }
    if (systs_[i]->shape_u() && systs_[i]->shape_d()) {
      // These hists will be normalised to unity
      std::unique_ptr<TH1> copy_u(systs_[i]->ClonedShapeU());
      std::unique_ptr<TH1> copy_d(systs_[i]->ClonedShapeD());

      // If we found a matching Process we will scale this back up to their
      // initial rates
      if (proc_hist) {
        copy_u->Scale(systs_[i]->value_u() * prev_rate);
        copy_d->Scale(systs_[i]->value_d() * prev_rate);
      }
      std::unique_ptr<TH1> copy2_u(
          copy_u->Rebin(bins.size() - 1, "", &(bins[0])));
      std::unique_ptr<TH1> copy2_d(
          copy_d->Rebin(bins.size() - 1, "", &(bins[0])));
      // If we have proc_hist != nullptr, set_shapes will re-calculate value_u
      // and value_d for us, before scaling the new hists back to unity
      systs_[i]->set_shapes(std::move(copy2_u), std::move(copy2_d), proc_hist);
    }
  }
}

void CombineHarvester::SetPdfBins(unsigned nbins) {
  for (unsigned i = 0; i < procs_.size(); ++i) {
    std::set<std::string> binning_vars;
    if (procs_[i]->pdf()) {
      RooAbsData const* data_obj = FindMatchingData(procs_[i].get());
      std::string var_name = "CMS_th1x";
      if (data_obj) var_name = data_obj->get()->first()->GetName();
      binning_vars.insert(var_name);
    }
    for (auto & it : wspaces_) {
      for (auto & var : binning_vars) {
        RooRealVar* avar =
            dynamic_cast<RooRealVar*>(it.second->var(var.c_str()));
        if (avar) avar->setBins(nbins);
      }
    }
  }
}
}
