#include "stdio.h"
#include "stdlib.h"
#include "time.h"


void evio_comparison()
{
	// Timer (for curiosity)
	clock_t start = clock();
	
	// Open up the Rootfiles we care about
	TFile* fprex = new TFile("/cache/halla/parity/rootfiles/pass1/crex_respin2/prexPrompt_pass2_5408.000.root");
	TFile* ffork = new TFile("/volatile/halla/moller12gev/mrconaway/rootfiles/CODA2/prexPrompt_pass1_evio_5408.000.root");
	TFile* fjapan= new TFile("/volatile/halla/moller12gev/mrconaway/rootfiles/JAPAN-MOLLER/prexPrompt_pass1_evio_5408.000.root");
	
	// Branch that we want to access
	const char* cbname = "bpm4ecXP";
	//const char* cbname = "bcm_dg_ds";
	// Each branch has 13 leaves
	double dvalPrex[13], dvalFork[13], dvalJapa[13];

	// Create Tree Objects
	TTree* tprexevt = (TTree*)fprex->Get("evt");
	TTree* tforkevt = (TTree*)ffork->Get("evt");
	TTree* tjapaevt = (TTree*)fjapan->Get("evt");
	
	// Provide variable address to access leaves in designated branch
	tprexevt->SetBranchAddress(cbname, &dvalPrex);
	tforkevt->SetBranchAddress(cbname, &dvalFork);
	tjapaevt->SetBranchAddress(cbname, &dvalJapa);

	// Entry Count
	size_t llPrexSiz = tprexevt->GetEntries();
	size_t llForkSiz = tforkevt->GetEntries();
	size_t llJapaSiz = tprexevt->GetEntries();

	// For floating point comparison
	float tol = 0.0001;
	printf("size = %zu\n",llPrexSiz);	

	// Create Graphs to plot Differences
	TGraph *gpf = new TGraph();
	TGraph *gpj = new TGraph();
	gpf->SetTitle("Prex & Fork;Event Num; Difference");
	gpj->SetTitle("Prex & japan-MOLLER;Event Num; Difference");
	gpf->SetMarkerStyle(kFullCircle);
	gpj->SetMarkerStyle(kFullCircle);

	// Only workds if each TTree has the same # of entries
	if( (llPrexSiz != llForkSiz) || (llPrexSiz != llJapaSiz) ){ printf("Size of Trees Do Not Match!"); exit(EXIT_FAILURE); }
	// Loop Over Entries
	for(size_t i = 0; i < llPrexSiz; i++){
		if( i%10000 == 0 ) printf("%zu Searched\n",i);
		// Load successive leaf rows from the branch
		tprexevt->GetEntry(i);
	  tforkevt->GetEntry(i);
    tjapaevt->GetEntry(i);
		// We only care about the first leaf in the leaf row
		double diffpf = dvalPrex[0] - dvalFork[0];
		double diffpj = dvalPrex[0] - dvalJapa[0];
		// If they are different, plot
		if( abs(diffpf) > tol){
			gpf->SetPoint(gpf->GetN(), i, diffpf);
		}
		if( abs(diffpj) > tol){
			gpj->SetPoint(gpj->GetN(), i, diffpj);
		}
	}

	// Only draw a canvas if we have points to plot
	TCanvas *c1 = new TCanvas();
	c1->Divide(1,2);
	c1->cd(1);
	if(gpf->GetN()){ gpf->Draw(); }
	c1->cd(2);
	if(gpj->GetN()){ gpj->Draw(); }
	if(gpf->GetN() == 0 && gpj->GetN() == 0) delete c1;

	// timer (for curiosity)
	clock_t duration = clock() - start;
	printf("Time Elapsed: %ldms\n",duration*1000/CLOCKS_PER_SEC);
	
	return 0;
}
