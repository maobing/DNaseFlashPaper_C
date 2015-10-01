#include<stdio.h>
#include<ctype.h>
#include<stdlib.h>
#include<string.h>
#include<errno.h>
#include "./liblinear-1.96/linear.h"
#include "DNaseFlash.h"

/* use liblinear API instead of system
 * remove the -m cmd option 
 * as only penalized logistic regression will be used
 */

int menu_predictModel(int argc, char **argv);
void do_predict(FILE *, FILE *);

/* some code from predict.c from liblinear */

int print_null(const char *s,...) {return 0;}

static int (*info)(const char *fmt,...) = &printf;

struct feature_node *x;
int max_nr_attr = 64;

struct model* model_;
int flag_predict_probability=0;

void exit_input_error(int line_num) {
  fprintf(stderr,"Wrong input format at line %d\n", line_num);
  exit(1);
}

static char *line = NULL;
static int max_line_len;

static char* readline(FILE *input) {
  int len;

  if(fgets(line,max_line_len,input) == NULL)
    return NULL;

  while(strrchr(line,'\n') == NULL){
      max_line_len *= 2;
      line = (char *) realloc(line,max_line_len);
      len = (int) strlen(line);
      if(fgets(line+len,max_line_len-len,input) == NULL)
        break;
  }
  return line;
}


/*------------------------*/
/*  predictModel          */
/*------------------------*/
int main(int argc, char **argv) {

  /* menu */
  menu_predictModel(argc, argv);

  /* exit */
  exit(EXIT_SUCCESS);
  
}



int menu_predictModel(int argc, char **argv) {

  /* ------------------------------- */
  /*        predictModel             */
  /* -m trainedModel                */
  /* -tr trainingFile             */
  /* -tt testingFile               */
  /* -o output results               */
  /* ------------------------------- */

  if (argc == 1) {
    printf("/*------------------------------------*/\n");
    printf("/*           menu_predictModel        */\n");
    printf("/* -m trainedModel                   */\n");
    printf("/* -tr trainFile                   */\n");
    printf("/* -tt testFile                     */\n");
    printf("/* -o output results                  */\n");
    printf("/*------------------------------------*/\n");
    exit(EXIT_SUCCESS);
  }

  char *trainedModel = (char *)calloc(MAX_DIR_LEN, sizeof(char));
  char *trainFile = (char *)calloc(MAX_DIR_LEN, sizeof(char)); 
  char *testFile = (char *)calloc(MAX_DIR_LEN, sizeof(char));
  char *outputFile = (char *)calloc(MAX_DIR_LEN, sizeof(char));

  int ni;
  int mOK = 0, trainOK = 0, testOK = 0, oOK = 0;

  ni = 1;
  while (ni < argc) {
    if (strcmp(argv[ni], "-m") == 0){
      ni++;
      strcpy(trainedModel, argv[ni]);
      mOK = 1;
    }
    else if (strcmp(argv[ni], "-tr") == 0){
      ni++;
      strcpy(trainFile, argv[ni]);
      trainOK = 1;
    }
    else if (strcmp(argv[ni], "-tt") == 0){
      ni++;
      strcpy(testFile, argv[ni]);
      testOK = 1;
    }
    else if (strcmp(argv[ni], "-o") == 0){
      ni++;
      strcpy(outputFile, argv[ni]);
      oOK = 1;
    }
    else {
      printf("Error: unkown parameters!\n");
      exit(EXIT_FAILURE);
    }
    ni++;
  }

  /* check args */
  if ((mOK + trainOK + testOK + oOK) < 4){
    printf("Error: input arguments not correct!\n");
    exit(EXIT_FAILURE);
  }
  else {
    FILE *input = fopen(testFile, "r");
    FILE *output = fopen(outputFile, "w");
    do_predict(input, output);
  }

  /* free pointers */
  free(trainedModel);
  free(trainFile);
  free(testFile);
  free(outputFile);

  return 0;
}

/* do_predict function from liblinear */
void do_predict(FILE *input, FILE *output) {
  int correct = 0;
  int total = 0;
  double error = 0;
  double sump = 0, sumt = 0, sumpp = 0, sumtt = 0, sumpt = 0;

  int nr_class=get_nr_class(model_);
  double *prob_estimates=NULL;
  int j, n;
  int nr_feature=get_nr_feature(model_);
  if(model_->bias>=0)
    n=nr_feature+1;
  else
    n=nr_feature;

  if(flag_predict_probability) {
    int *labels;

    if(!check_probability_model(model_)) {
      fprintf(stderr, "probability output is only supported for logistic regression\n");
      exit(1);
    }

    labels=(int *) malloc(nr_class*sizeof(int));
    get_labels(model_,labels);
    prob_estimates = (double *) malloc(nr_class*sizeof(double));
    fprintf(output,"labels");
    for(j=0;j<nr_class;j++)
      fprintf(output," %d",labels[j]);
    fprintf(output,"\n");
    free(labels);
  }

  max_line_len = 1024;
  line = (char *)malloc(max_line_len*sizeof(char));
  while(readline(input) != NULL) {
    int i = 0;
    double target_label, predict_label;
    char *idx, *val, *label, *endptr;
    int inst_max_index = 0; // strtol gives 0 if wrong format

    label = strtok(line," \t\n");
    if(label == NULL) // empty line
      exit_input_error(total+1);

    target_label = strtod(label,&endptr);
    if(endptr == label || *endptr != '\0')
      exit_input_error(total+1);

    while(1) {
      if(i>=max_nr_attr-2) { // need one more for -1
        max_nr_attr *= 2;
        x = (struct feature_node *) realloc(x,max_nr_attr*sizeof(struct feature_node));
      }

      idx = strtok(NULL,":");
      val = strtok(NULL," \t");

      if(val == NULL)
        break;
      errno = 0;
      x[i].index = (int) strtol(idx,&endptr,10);
      if(endptr == idx || errno != 0 || *endptr != '\0' || x[i].index <= inst_max_index)
        exit_input_error(total+1);
      else
        inst_max_index = x[i].index;

      errno = 0;
      x[i].value = strtod(val,&endptr);
      if(endptr == val || errno != 0 || (*endptr != '\0' && !isspace(*endptr)))
        exit_input_error(total+1);

      // feature indices larger than those in training are not used
      if(x[i].index <= nr_feature)
        ++i;
    }

    if(model_->bias >= 0) {
      x[i].index = n;
      x[i].value = model_->bias;
      i++;
    }
    x[i].index = -1;

    if(flag_predict_probability) {
      int j;
      predict_label = predict_probability(model_,x,prob_estimates);
      fprintf(output,"%g",predict_label);
      for(j=0;j<model_->nr_class;j++)
        fprintf(output," %g",prob_estimates[j]);
      fprintf(output,"\n");
    } else {
    predict_label = predict(model_,x);
    fprintf(output,"%g\n",predict_label);
    }

    if(predict_label == target_label)
      ++correct;
    error += (predict_label-target_label)*(predict_label-target_label);
    sump += predict_label;
    sumt += target_label;
    sumpp += predict_label*predict_label;
    sumtt += target_label*target_label;
    sumpt += predict_label*target_label;
    ++total;
  }
  if(check_regression_model(model_)) {
    info("-Mean squared error = %g (regression)\n",error/total);
    info("Squared correlation coefficient = %g (regression)\n",
      ((total*sumpt-sump*sumt)*(total*sumpt-sump*sumt))/
      ((total*sumpp-sump*sump)*(total*sumtt-sumt*sumt)));
  }
  else
    info("Accuracy = %g%% (%d/%d)\n",(double) correct/total*100,correct,total);

  if(flag_predict_probability)
    free(prob_estimates);
}



