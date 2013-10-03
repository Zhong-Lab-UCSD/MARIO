#!/usr/bin/env Rscript

list.of.packages <- c("argparse","RCircos", "biovizBase","rtracklayer")
new.packages <- list.of.packages[!(list.of.packages %in% installed.packages()[,"Package"])]
if(length(new.packages)) install.packages(new.packages,repos="http://R-Forge.R-project.org")
suppressPackageStartupMessages(require(rtracklayer))
suppressPackageStartupMessages(require(RCircos))
suppressPackageStartupMessages(require(argparse))

# creat parser object
parser <- ArgumentParser()

# specify options
parser$add_argument("interaction",help="the interaction file,[required]")
parser$add_argument("-g","--genome",default="mm9",help="genome information, choice: mm9/mm10/hg19 et.al., [default: %(default)s]")
parser$add_argument("part1",help="aligned BAM file for part1,[required]")
parser$add_argument("part2",help="aligned BAM file for part2,[required]")
parser$add_argument("-b","--bin",type="integer",default=100000,help="window size for the bins, [default: %(default)s]")
parser$add_argument("-o","--output",default="Interactome_view.pdf",help="output pdf file name, [default: %(default)s]")



#  modified Histogram function add color option
RCircos.Histogram.Plot<-function(hist.data, data.col, track.num, side, col)
{
  RCircos.Pos <- RCircos.Get.Plot.Positions();
  RCircos.Par <- RCircos.Get.Plot.Parameters();
  RCircos.Cyto <- RCircos.Get.Plot.Ideogram();
  
  
  #       Convert raw data to plot data. The raw data will be validated
  #       first during the convertion
  #       ____________________________________________________________
  #       xxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx
  
  hist.data <- RCircos.Get.Plot.Data(hist.data, "plot");
  
  
  #       Each heatmap cell is centered on data point location. Make
  #       sure each one will be in the range of it chromosome
  #       ____________________________________________________________
  #       xxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx
  
  hist.locations <- as.numeric(hist.data[, ncol(hist.data)]);
  start <- hist.locations - RCircos.Par$hist.width;
  end   <- hist.locations + RCircos.Par$hist.width;
  
  data.chroms <- as.character(hist.data[,1]);
  chromosomes <- unique(data.chroms);
  cyto.chroms <- as.character(RCircos.Cyto$Chromosome);
  
  for(a.chr in 1:length(chromosomes))
  {
    cyto.rows <- which(cyto.chroms==chromosomes[a.chr]);
    locations <- as.numeric(RCircos.Cyto$Location[cyto.rows]);
    chr.start <- min(locations) - RCircos.Cyto$Unit[cyto.rows[1]];
    chr.end   <- max(locations);
    
    data.rows <- which(data.chroms==chromosomes[a.chr]);
    start[data.rows[start[data.rows]<chr.start]] <- chr.start;
    end[data.rows[end[data.rows]>chr.end]] <- chr.end;
  }
  
  
  #       Plot position for current track. 
  #       ___________________________________________________________
  #       xxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx
  
  locations <- RCircos.Track.Positions(side, track.num);
  out.pos <- locations[1];
  in.pos <- locations[2];
  
  
  #       Draw histogram [modified]
  #       ___________________________________________________________
  #       xxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx
  
  num.subtrack <- RCircos.Par$sub.tracks;
  RCircos.Track.Outline(out.pos, in.pos, RCircos.Par$sub.tracks);
  
  for(a.point in 1:nrow(hist.data))
  {
    hist.height <- hist.data[a.point, data.col];
    
    the.start <- start[a.point];
    the.end <- end[a.point];
    
    #       Plot rectangle with specific height for each 
    #       data point
    #       _______________________________________________
    #       xxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx
    
    height <- in.pos + RCircos.Par$track.height*hist.height;
    
    polygon.x<- c(RCircos.Pos[the.start:the.end,1]*height, 
                  RCircos.Pos[the.end:the.start,1]*in.pos);
    polygon.y<- c(RCircos.Pos[the.start:the.end,2]*height, 
                  RCircos.Pos[the.end:the.start,2]*in.pos);
    polygon(polygon.x, polygon.y, col=col, border=NA);
  }
  
}

#  modified link plot function change color scheme (color_list is the vector of p-values for all interactions)
RCircos.Link.Plot<-function(link.data, track.num, color_list)
{
  RCircos.Pos <- RCircos.Get.Plot.Positions();
  RCircos.Par <- RCircos.Get.Plot.Parameters();
  
  
  #       Check chromosome names, Start, and End positions
  #       _______________________________________________________________
  #       xxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx
  
  link.data <- RCircos.Validate.Genomic.Data(link.data, plot.type="link");
  
  
  #       Plot position for link track.
  #       __________________________________________________________
  #       xxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx
  
  one.track <- RCircos.Par$track.height + RCircos.Par$track.padding;
  start <- RCircos.Par$track.in.start - (track.num-1)*one.track;
  base.positions <- RCircos.Pos*start;
  
  data.points <- matrix(rep(0, nrow(link.data)*2), ncol=2);
  colfunc <- colorRampPalette(c("white", "red"));
  line.colors <- colfunc(12)[-2*log10(color_list+1e-6)]
  for(a.link in 1:nrow(link.data))
  {
    data.points[a.link, 1] <- RCircos.Data.Point(
      link.data[a.link, 1], link.data[a.link, 2]);
    data.points[a.link, 2] <- RCircos.Data.Point(
      link.data[a.link, 4], link.data[a.link, 5]);
    
    if(data.points[a.link, 1]==0 || data.points[a.link, 2]==0)
    {  print("Error in chromosome locations ...");  break; }
    
  }
  
  
  #       Draw link lines for each pair of locations
  #       __________________________________________________
  #       xxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx
  
  for(a.link in 1:nrow(data.points))
  {  
    point.one <- data.points[a.link, 1];
    point.two <- data.points[a.link, 2];
    if(point.one > point.two)
    { 
      point.one <- data.points[a.link, 2];
      point.two <- data.points[a.link, 1];
    }
    
    P0 <- as.numeric(base.positions[point.one,]);
    P2 <- as.numeric(base.positions[point.two,]);
    links <- RCircos.Link.Line(P0, P2); 
    lines(links$pos.x, links$pos.y, type="l", 
          col=line.colors[a.link] ); 
  }
}

RCircos.Link.Line<-function(line.start, line.end)
{       
  #       Set up the points for Bezure curve
  #       ___________________________________________
  #       xxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx
  
  P0 <- line.start;
  P2 <- line.end;
  
  
  #       Calculate total number of points for 
  #       the Bezuer curve
  #       ___________________________________________
  #       xxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx
  
  bc.point.num <- 1000;
  t <- seq(0, 1, 1/bc.point.num);
  
  
  #       Calculate the point values for Bezuer curve
  #       ___________________________________________
  #       xxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx
  
  link.x <- (1-t)^2*P0[1] + t^2*P2[1];
  link.y <- (1-t)^2*P0[2] + t^2*P2[2];
  
  
  #       Return the coordinates
  #       ___________________________________________
  #       xxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx
  
  return (list(pos.x=link.x, pos.y=link.y));
}



# get the input
args <- parser$parse_args()
interaction_f <- args$interaction
genome <- args$genome
part1 <- args$part1
part2 <- args$part2
bin <- args$bin
output <- args$output

# get cytoband ideogram information
cat ("Generating cytoband information from genome version... \n")
if (genome=="mm10") {
  data(UCSC.Mouse.GRCm38.CytoBandIdeogram)
  cytoband=UCSC.Mouse.GRCm38.CytoBandIdeogram
  rm(UCSC.Mouse.GRCm38.CytoBandIdeogram)
} else if (genome=="hg19") {
  data(UCSC.HG19.Human.CytoBandIdeogram)
  cytoband=UCSC.HG19.Human.CytoBandIdeogram
  rm(UCSC.HG19.Human.CytoBandIdeogram)
} else {
  suppressPackageStartupMessages(require(biovizBase))
  cytoband=as.data.frame(getIdeogram(genome,cytobands=T))
  cytoband=cytoband[,c(1:3,6:7)]
  colnames(cytoband)=c("Chromosome","ChromStart","ChromEnd","Band","Stain")
}


# get coverage for BAM files of part1 and part2
cat("\nCounting coverage (RPKM) for part1 and part2 alignment:")
commend <- paste("bam2tab.py --bin",bin,"-s",part1,part2,"> temp_coverage.txt",sep=" ")
system(commend)
coverage=read.table("temp_coverage.txt",header=F)
coverage=coverage[coverage[,1]!="chrM",]
# for windows exceed the chromosome boundary
for (chr in unique(as.character(coverage[,1]))) {
  n=max((1:dim(coverage)[1])[coverage[,1]==chr])
  m=max((1:dim(cytoband)[1])[cytoband$Chromosome==chr])
  coverage[n,3]=min(coverage[n,3],cytoband[m,3])
}

part1_cov=coverage[,1:4]
part2_cov=coverage[,c(1:3,5)]
colnames(part1_cov)=c("Chromosome", "chromStart", "chromEnd", "Data")
colnames(part2_cov)=c("Chromosome", "chromStart", "chromEnd", "Data")
system("rm temp_coverage.txt")
part1_cov$Data=log(part1_cov$Data+1)/max(log(part1_cov$Data+1))
part2_cov$Data=log(part2_cov$Data+1)/max(log(part2_cov$Data+1))

# get interaction file ready
cat("Generating link data from interaction file... \n")
interaction<-read.table(interaction_f,sep='\t',header=F)
interaction=interaction[(!grepl("rRNA",interaction[,4]))&(!grepl("rRNA",interaction[,11])),]
interaction_p=interaction[,16]
interaction=interaction[,c(1:3,8:10)]
data(RCircos.Link.Data)
colnames(interaction)<-colnames(RCircos.Link.Data)
interaction=interaction[(interaction[,1]!="chrM")&(interaction[,4]!="chrM"),]
rm(RCircos.Link.Data)


#  Setup RCircos core components:
RCircos.Set.Core.Components(cytoband, NULL, 2, 0);

#  Open the graphic device (here a pdf file)
cat("Open graphic device and start plot ...\n");
pdf(file=output, height=8, width=8);
RCircos.Set.Plot.Area();
title("RNA-RNA interactome");

# 	Draw chromosome ideogram
cat("Draw chromosome ideogram ...\n");
RCircos.Chromosome.Ideogram.Plot();

#  Histogram plot for part1 and part2
cat("Draw Histogram of part1/2 ...\n");
RCircos.Histogram.Plot(part1_cov, 4, 1, "in","#763a7a");
RCircos.Histogram.Plot(part2_cov, 4, 2, "in","#0288ad");

#  Link plot for interactions
cat("Draw Interaction links ...\n");
RCircos.Link.Plot(interaction,3,interaction_p)

#  Close the graphic device and clear memory
dev.off()
cat("Circos interaction plotting done ...\n\n");
rm(list=ls(all=T));
