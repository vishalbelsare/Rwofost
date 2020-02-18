#include <math.h>
#include <vector>
#include "wofost.h"
#include "SimUtil.h"
#include <string.h>

/*
// Copyright (c) 2004-2015 Alterra, Wageningen-UR
// Allard de Wit and Iwan Supit (allard.dewit@wur.nl), July 2015
// Approach based on LINTUL N/P/K made by Joost Wolf

"""
Class to calculate various nutrient relates stress factors:
    NNI      nitrogen nutrition index
    PNI      phosphorous nutrition index
    KNI      potap.SSIum nutrition index
    NPKI     NPK nutrition index (= minimum of N/P/K-index)
    NPKREF   assimilation reduction factor based on NPKI
"""

    """Implementation of NPK stress calculation through [NPK]nutrition index.

    Stress factors are calculated based on the mass concentrations of N/P/K in
    the leaf and stem biomass of the plant. For each pool of nutrients, four
    concentrations are calculated based on the biomass for leaves and stems:
    - the actual concentration based on the actual amount of nutrients
      divided by the actual leaf and stem biomass.
    - The maximum concentration, being the maximum that the plant can absorb
      into its leaves and stems.
    - The critical concentration, being the concentration that is needed to
      maintain growth rates that are not limited by N/P/K. For P and K, the
      critical concentration is usually equal to the maximum concentration.
      For N, the critical concentration can be lower than the maximum
      concentration. This concentration is sometimes called 'optimal
      concentration'.
    - The residual concentration which is the amount that is locked
      into the plant structural biomass and cannot be mobilized anymore.

    The stress index (SI) is determined as a simple ratio between those
    concentrations according to:

    :math:`SI = (C_{a) - C_{r})/(C_{c} - C_{r})`

    with subscript `a`, `r` and `c` being the actual, residual and critical
    concentration for the nutrient.
    This equation is applied in analogue to N, P and K and results in the
    nitrogen nutrition index (NNI), phosphorous nutrition index (PNI) and
    Potap.SSIum nutrition index (KNI). Next, the NPK index (NPKI) is calculated
    as the minimum of NNI, PNI, KNI. Finally, the reduction factor for
    assimilation (NPKREF) is calculated using the reduction factor for
    light use efficiency (NLUE_NPK).

    **Simulation parameters**

    ============  ============================================= =======  ======================
     Name          Description                                   Type     Unit
    ============  ============================================= =======  ======================
    NMAXLV_TB      Maximum N concentration in leaves as          TCr     kg N kg-1 dry biomass
                   function of DVS
    PMAXLV_TB      As for P                                      TCr     kg P kg-1 dry biomass
    KMAXLV_TB      As for K                                      TCr     kg K kg-1 dry biomass

    NMAXRT_FR      Maximum N concentration in roots as fraction  SCr     -
                   of maximum N concentration in leaves
    PMAXRT_FR      As for P                                      SCr     -
    KMAXRT_FR      As for K                                      SCr     -

    NMAXST_FR      Maximum N concentration in stems as fraction  SCr     -
                   of maximum N concentration in leaves
    PMAXST_FR      As for P                                      SCr     -
    KMAXST_FR      As for K                                      SCr     -

    NCRIT_FR       Critical N concentration as fraction of       SCr     -
                   maximum N concentration for vegetative
                   plant organs as a whole (leaves + stems)
    PCRIT_FR       As for P                                      SCr     -
    KCRIT_FR       As for K                                      SCr     -

    NRESIDLV       Residual N fraction in leaves                 SCr     kg N kg-1 dry biomass
    PRESIDLV       Residual P fraction in leaves                 SCr     kg P kg-1 dry biomass
    KRESIDLV       Residual K fraction in leaves                 SCr     kg K kg-1 dry biomass

    NRESIDST       Residual N fraction in stems                  SCr     kg N kg-1 dry biomass
    PRESIDST       Residual P fraction in stems                  SCr     kg P kg-1 dry biomass
    KRESIDST       Residual K fraction in stems                  SCr     kg K kg-1 dry biomass

    NLUE_NPK       Coefficient for the reduction of RUE due      SCr     -
                   to nutrient (N-P-K) stress
    ============  ============================================= ======= =======================

    **Rate variables**

    The rate variables here are not real rate variables in the sense that they are derived
    state variables and do not represent a rate. However, as they are directly used
    in the rate variable calculation it is logical to put them here.

    =======  ================================================= ==== ==============
     Name     Description                                      Pbl      Unit
    =======  ================================================= ==== ==============
    NNI       Nitrogen nutrition index                          Y     -
    PNI       Nitrogen nutrition index                          N     -
    KNI       Nitrogen nutrition index                          N     -
    NPKI      Minimum of NNI, PNI, KNI                          Y     -
    NPKREF    Reduction factor for |CO2| assimlation            N     -
              based on NPKI and the parameter NLUE_NPK
    =======  ================================================= ==== ==============


    **External dependencies:**

    =========  =================================== =====================  ==============
     Name       Description                         Provided by            Unit
    =========  =================================== =====================  ==============
    DVS         Crop development stage              DVS_Phenology           -
    WST         Dry weight of living stems          WOFOST_Stem_Dynamics  |kg ha-1|
    WLV         Dry weight of living leaves         WOFOST_Leaf_Dynamics  |kg ha-1|
    ANLV        Amount of N in leaves               NPK_Crop_Dynamics     |kg ha-1|
    ANST        Amount of N in stems                NPK_Crop_Dynamics     |kg ha-1|
    APLV        Amount of P in leaves               NPK_Crop_Dynamics     |kg ha-1|
    APST        Amount of P in stems                NPK_Crop_Dynamics     |kg ha-1|
    AKLV        Amount of K in leaves               NPK_Crop_Dynamics     |kg ha-1|
    AKST        Amount of K in stems                NPK_Crop_Dynamics     |kg ha-1|
    =========  =================================== =====================  ==============
    """

    class Parameters(ParamTemplate):
        NMAXLV_TB = AfgenTrait()  // maximum N concentration in leaves as function of dvs
        PMAXLV_TB = AfgenTrait()  // maximum P concentration in leaves as function of dvs
        KMAXLV_TB = AfgenTrait()  // maximum P concentration in leaves as function of dvs
        NCRIT_FR = Float(-99.)   // optimal N concentration as fraction of maximum N concentration
        PCRIT_FR = Float(-99.)   // optimal P concentration as fraction of maximum P concentration
        KCRIT_FR = Float(-99.)   // optimal K concentration as fraction of maximum K concentration
        NMAXRT_FR = Float(-99.)  // maximum N concentration in roots as fraction of maximum N concentration in leaves
        NMAXST_FR = Float(-99.)  // maximum N concentration in stems as fraction of maximum N concentration in leaves
        PMAXST_FR = Float(-99.)  // maximum P concentration in roots as fraction of maximum P concentration in leaves
        PMAXRT_FR = Float(-99.)  // maximum P concentration in stems as fraction of maximum P concentration in leaves
        KMAXRT_FR = Float(-99.)  // maximum K concentration in roots as fraction of maximum K concentration in leaves
        KMAXST_FR = Float(-99.)  // maximum K concentration in stems as fraction of maximum K concentration in leaves
        NRESIDLV = Float(-99.)  // residual N fraction in leaves [kg N kg-1 dry biomass]
        NRESIDST = Float(-99.)  // residual N fraction in stems [kg N kg-1 dry biomass]
        PRESIDLV = Float(-99.)  // residual P fraction in leaves [kg P kg-1 dry biomass]
        PRESIDST = Float(-99.)  // residual P fraction in stems [kg P kg-1 dry biomass]
        KRESIDLV = Float(-99.)  // residual K fraction in leaves [kg K kg-1 dry biomass]
        KRESIDST = Float(-99.)  // residual K fraction in stems [kg K kg-1 dry biomass]
        NLUE_NPK = Float(-99.)  // coefficient for the reduction of RUE due to nutrient (N-P-K) stress

*/



void WofostModel::npk_stress() {

// Maximum NPK concentrations in leaves (kg N kg-1 DM)
    double NMAXLV = AFGEN(crop.pn.NMAXLV_TB, crop.DVS);
    double PMAXLV = AFGEN(crop.pn.PMAXLV_TB, crop.DVS);
    double KMAXLV = AFGEN(crop.pn.KMAXLV_TB, crop.DVS);

// Maximum NPK concentrations in stems (kg N kg-1 DM)
    double NMAXST = crop.pn.NMAXST_FR * NMAXLV;
    double PMAXST = crop.pn.PMAXRT_FR * PMAXLV;
    double KMAXST = crop.pn.KMAXST_FR * KMAXLV;


// Total vegetative living above-ground biomass (kg DM ha-1)
    double TBGMR = crop.WLV + crop.WST ;

// Critical (Optimal) NPK amount in vegetative above-ground living biomass
// and its NPK concentration
    double NCRITLV  = crop.pn.NCRIT_FR * NMAXLV * crop.WLV;
    double NCRITST  = crop.pn.NCRIT_FR * NMAXST * crop.WST;

    double PCRITLV = crop.pn.PCRIT_FR * PMAXLV * crop.WLV;
    double PCRITST = crop.pn.PCRIT_FR * PMAXST * crop.WST;

    double KCRITLV = crop.pn.KCRIT_FR * KMAXLV * crop.WLV;
    double KCRITST = crop.pn.KCRIT_FR * KMAXST * crop.WST;

// if above-ground living biomass = 0 then optimum = 0
	double NCRITMR, PCRITMR, KCRITMR;
    if (TBGMR > 0) {
        NCRITMR = (NCRITLV + NCRITST)/TBGMR;
        PCRITMR = (PCRITLV + PCRITST)/TBGMR;
        KCRITMR = (KCRITLV + KCRITST)/TBGMR;
    } else {
        NCRITMR = 0.;
				PCRITMR = 0.;
				KCRITMR = 0.;
	}

// NPK concentration in total vegetative living per kg above-ground biomass  (kg N/P/K kg-1 DM)
// if above-ground living biomass = 0 then concentration = 0
	double NFGMR, PFGMR, KFGMR;
    if (TBGMR > 0) {
        NFGMR  = (crop.sn.ANLV + crop.sn.ANST)/TBGMR;
        PFGMR  = (crop.sn.APLV + crop.sn.APST)/TBGMR;
        KFGMR  = (crop.sn.AKLV + crop.sn.AKST)/TBGMR;
    } else {
        NFGMR = PFGMR = KFGMR = 0.;
	}

    // Residual NPK concentration in total vegetative living above-ground
    // biomass  (kg N/P/K kg-1 DM)
    // if above-ground living biomass = 0 then residual concentration = 0
	double NRMR, PRMR, KRMR;
    if (TBGMR > 0.) {
        NRMR = (crop.WLV * crop.pn.NRESIDLV + crop.WST * crop.pn.NRESIDST)/TBGMR;
        PRMR = (crop.WLV * crop.pn.PRESIDLV + crop.WST * crop.pn.PRESIDST)/TBGMR;
        KRMR = (crop.WLV * crop.pn.KRESIDLV + crop.WST * crop.pn.KRESIDST)/TBGMR;
    } else {
        NRMR = 0.;
				PRMR = 0.;
				KRMR = 0.;
    }


    if ((NCRITMR - NRMR) > 0.) {
        crop.vn.NNI = LIMIT(0.001, 1.0, (NFGMR-NRMR)/(NCRITMR-NRMR));
    } else {
        crop.vn.NNI = 0.001;
    }

    if ((PCRITMR - PRMR) > 0.) {
        crop.vn.PNI = LIMIT(0.001, 1.0, (PFGMR-PRMR)/(PCRITMR-PRMR));
    } else {
       crop.vn.PNI = 0.001;
    }

    if ((KCRITMR-KRMR) > 0) {
        crop.vn.KNI = LIMIT(0.001, 1.0, (KFGMR-KRMR)/(KCRITMR-KRMR));
    } else {
        crop.vn.KNI = 0.001;
	}

    crop.vn.NPKI = std::min( std::min(crop.vn.NNI, crop.vn.PNI), crop.vn.KNI);

    // Nutrient reduction factor for assimilation
    crop.vn.NPKREF = LIMIT(0., 1.0, 1. - (crop.pn.NLUE_NPK * pow((1.0001-crop.vn.NPKI), 2)));

//    return crop.rn.NNI, crop.rn.NPKI, crop.rn.NPKREF;
}
