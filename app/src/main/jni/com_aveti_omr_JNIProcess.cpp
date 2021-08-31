#include "com_aveti_omr_JNIProcess.h"

#define APPNAME "ImageProcessing"

std::string to_string(int i)
{
    std::stringstream ss;
    ss << i;
    return ss.str();
}

JNIEXPORT jfloatArray JNICALL Java_com_aveti_omr_JNIProcess_getPoints
  (JNIEnv *env, jlong addrRgba)
{
    Mat& mbgra = *(Mat*)addrRgba;
    vector<Point> img_pts = getPoints(mbgra);
    jfloatArray jArray = env->NewFloatArray(8);

    if (jArray != NULL)
    {
        jfloat *ptr = env->GetFloatArrayElements(jArray, NULL);

        for (int i=0,j=i+4; j<8; i++,j++)
        {
            ptr[i] = img_pts[i].x;
            ptr[j] = img_pts[i].y;
        }
        env->ReleaseFloatArrayElements(jArray, ptr, NULL);
    }
    return jArray;
}

double angle( cv::Point pt1, cv::Point pt2, cv::Point pt0 ) {
    double dx1 = pt1.x - pt0.x;
    double dy1 = pt1.y - pt0.y;
    double dx2 = pt2.x - pt0.x;
    double dy2 = pt2.y - pt0.y;
    return (dx1*dx2 + dy1*dy2)/sqrt((dx1*dx1 + dy1*dy1)*(dx2*dx2 + dy2*dy2) + 1e-10);
}

vector<Point> getPoints(Mat image)
{
    int width = image.size().width;
    int height = image.size().height;
    Mat image_proc = image.clone();
    vector<vector<Point> > squares;
    // blur will enhance edge detection
    Mat blurred(image_proc);
    medianBlur(image_proc, blurred, 5);

    Mat gray0(blurred.size(), CV_8U), gray;
    vector<vector<Point> > contours;

    // find squares in every color plane of the image
    for (int c = 0; c < 3; c++)
    {
        int ch[] = {c, 0};
        mixChannels(&blurred, 1, &gray0, 1, ch, 1);

        // try several threshold levels
        const int threshold_level = 2;
        for (int l = 0; l < threshold_level; l++)
        {
            // Use Canny instead of zero threshold level!
            // Canny helps to catch squares with gradient shading
            if (l == 0)
            {
                Canny(gray0, gray, 20, 100, 3); //

                // Dilate helps to remove potential holes between edge segments
                dilate(gray, gray, Mat(), Point(-1,-1));
            }
            else
            {
                gray = gray0 >= (l+1) * 255 / threshold_level;
            }

            // Find contours and store them in a list
            findContours(gray, contours, CV_RETR_LIST, CV_CHAIN_APPROX_SIMPLE);

            // Test contours
            vector<Point> approx;
            for (size_t i = 0; i < contours.size(); i++)
            {
                // approximate contour with accuracy proportional
                // to the contour perimeter
                approxPolyDP(Mat(contours[i]), approx, arcLength(Mat(contours[i]), true)*0.02, true);

                // Note: absolute value of an area is used because
                // area may be positive or negative - in accordance with the
                // contour orientation
                if (approx.size() == 4 &&
                    fabs(contourArea(Mat(approx))) > 10000)
                {
                    double maxCosine = 0;

                    for (int j = 2; j < 5; j++)
                    {
                        double cosine = fabs(angle(approx[j%4], approx[j-2], approx[j-1]));
                        maxCosine = MAX(maxCosine, cosine);
                    }

                    if (maxCosine < 0.3)
                        squares.push_back(approx);
                }
            }
        }

        //get largest contour
        double largest_area = -1;
        int largest_contour_index = 0;
        for(int i=0;i<squares.size();i++)
        {
            double a =contourArea(squares[i],false);
            if(a>largest_area)
            {
                largest_area = a;
                largest_contour_index = i;
            }
        }

        vector<Point> points;
        if(squares.size() > 0)
        {
            points = squares[largest_contour_index];
        }
        else
        {
            points.push_back(Point(0, 0));
            points.push_back(Point(0, 0));
            points.push_back(Point(0, 0));
            points.push_back(Point(0, 0));
        }

        return points;
    }
}

JNIEXPORT jstring JNICALL Java_com_aveti_omr_JNIProcess_recogBubble
  (JNIEnv *env,jlong inputMat)
{
    Mat& image = *(Mat*)inputMat;
    cv::Mat gray,thresh;
    std::vector<std::vector<cv::Point> > contours;
    std::vector<cv::Vec4i> hierarchy;

    cvtColor(image, gray, cv::COLOR_BGR2GRAY);
    adaptiveThreshold(gray, thresh, 255, CV_ADAPTIVE_THRESH_GAUSSIAN_C, CV_THRESH_BINARY_INV, 101, 15);
    findContours(thresh, contours, hierarchy, CV_RETR_CCOMP, CV_CHAIN_APPROX_SIMPLE, cv::Point(0, 0));

    //don't forget to input space before >
    std::vector<std::vector<cv::Point> > contours_poly(contours.size());
    std::vector<cv::Rect> boundRect(contours.size());
    std::vector<std::vector<cv::Point> > rollCnt;
    std::vector<std::vector<cv::Point> > examCnt;
    std::vector<std::vector<cv::Point> > centerCnt;

    std::vector<std::vector<Point> > ansCnt1;
    std::vector<std::vector<Point> > ansCnt2;
    std::vector<std::vector<Point> > ansCnt3;
    std::vector<std::vector<Point> > ansCnt4;
    std::vector<std::vector<Point> > ansCnt5;

    ///Processing the CheckID
    for (int i = 0; i < contours.size(); i++)
    {
        approxPolyDP(cv::Mat(contours[i]), contours_poly[i], 0.2, true);
        int w = boundingRect(Mat(contours[i])).width;
        int h = boundingRect(Mat(contours[i])).height;
        int positionx = boundingRect(Mat(contours[i])).br().x;
        int positiony = boundingRect(Mat(contours[i])).br().y;

        if (hierarchy[i][3] == -1) {
            if (w >= 10 && h >= 10 && w <= 16 && h <= 16) {
                if (positiony > 75 && positiony < 240) {
                    if (positionx < 153) {
                        rollCnt.push_back(contours_poly[i]);
                    }
                    if (positionx >= 153 && positionx < 307) {
                        examCnt.push_back(contours_poly[i]);
                    }
                    if (positionx >= 307) {
                        centerCnt.push_back(contours_poly[i]);
                    }
                }

				if (positiony > 240) {
					if (positionx < 94) {
						ansCnt1.push_back(contours_poly[i]);
					}
					if (positionx >= 94 && positionx < 170) {
						ansCnt2.push_back(contours_poly[i]);
					}
					if (positionx >= 170 && positionx < 246) {
						ansCnt3.push_back(contours_poly[i]);
					}
					if (positionx >= 246 && positionx < 323) {
						ansCnt4.push_back(contours_poly[i]);
					}
					if (positionx >= 323) {
						ansCnt5.push_back(contours_poly[i]);
					}
				}
            }
        }
    }

    //sort rollcnt
    sort_contour(rollCnt, 0, (int)rollCnt.size(), std::string("left-to-right"));
    for (int i = 0; i < rollCnt.size(); i = i + 10)
    {
        int end = i + 10;
        if (end > rollCnt.size())
            end = (int)rollCnt.size();
        if (end + 1 == rollCnt.size()) {
            end = end + 1;
        }
        sort_contour(rollCnt, i, end, std::string("top-to-bottom"));
    }

    std::string rollno = "";
    for (int i = 0; i < rollCnt.size(); i++)
    {
        cv::Rect rectCrop = boundingRect(rollCnt[i]);
        cv::Mat imageROI(thresh, rectCrop);

        double total = cv::countNonZero(imageROI);
        double pixel = total / rectCrop.area() * 100;

        if (pixel > 65 && pixel < 100)
        {
            if( (i % 10) == 9){
                rollno = rollno + "0";
            }
            else{
                rollno = rollno + to_string((i % 10) + 1) ;
            }

            rectangle(image, rectCrop.tl(), rectCrop.br(), cv::Scalar(0, 255, 0), 2, 8, 0);
        }
        else{
            rectangle(image, rectCrop.tl(), rectCrop.br(), Scalar(0, 0, 255), 1, 8, 0);
        }
    }

    //sort examcnt
    sort_contour(examCnt, 0, (int)examCnt.size(), std::string("left-to-right"));
    for (int i = 0; i < examCnt.size(); i = i + 10)
    {
        int end = i + 10;
        if (end > examCnt.size())
            end = (int)examCnt.size();
        if (end + 1 == examCnt.size()) {
            end = end + 1;
        }
        sort_contour(examCnt, i, end, std::string("top-to-bottom"));
    }

    std::string examid = "";
    for (int i = 0; i < examCnt.size(); i++)
    {
        cv::Rect rectCrop = boundingRect(examCnt[i]);
        cv::Mat imageROI(thresh, rectCrop);

        double total = cv::countNonZero(imageROI);
        double pixel = total / rectCrop.area() * 100;

        if (pixel > 65 && pixel < 100)
        {
            if( (i % 10) == 9){
                examid = examid + "0";
            }
            else{
                examid = examid + to_string((i % 10) + 1) ;
            }
            rectangle(image, rectCrop.tl(), rectCrop.br(), cv::Scalar(0, 255, 0), 2, 8, 0);
        }
        else{
            rectangle(image, rectCrop.tl(), rectCrop.br(), Scalar(0, 0, 255), 1, 8, 0);
        }
    }

    //sort centercnt
    sort_contour(centerCnt, 0, (int)centerCnt.size(), std::string("left-to-right"));
    for (int i = 0; i < centerCnt.size(); i = i + 10)
    {
        int end = i + 10;
        if (end > centerCnt.size())
            end = (int)centerCnt.size();
        if (end + 1 == centerCnt.size()) {
            end = end + 1;
        }
        sort_contour(centerCnt, i, end, std::string("top-to-bottom"));
    }

    std::string centerid = "";
    for (int i = 0; i < centerCnt.size(); i++)
    {
        cv::Rect rectCrop = boundingRect(centerCnt[i]);
        cv::Mat imageROI(thresh, rectCrop);

        double total = cv::countNonZero(imageROI);
        double pixel = total / rectCrop.area() * 100;

        if (pixel > 65 && pixel < 100)
        {
            if( (i % 10) == 9){
                centerid = centerid + "0";
            }
            else{
                centerid = centerid + to_string((i % 10) + 1) ;
            }
            rectangle(image, rectCrop.tl(), rectCrop.br(), cv::Scalar(0, 255, 0), 2, 8, 0);
        }
        else{
            rectangle(image, rectCrop.tl(), rectCrop.br(), Scalar(0, 0, 255), 1, 8, 0);
        }
    }

    sort_contour(ansCnt1, 0, (int)ansCnt1.size(), std::string("top-to-bottom"));
    sort_contour(ansCnt2, 0, (int)ansCnt2.size(), std::string("top-to-bottom"));
    sort_contour(ansCnt3, 0, (int)ansCnt3.size(), std::string("top-to-bottom"));
    sort_contour(ansCnt4, 0, (int)ansCnt4.size(), std::string("top-to-bottom"));
    sort_contour(ansCnt5, 0, (int)ansCnt5.size(), std::string("top-to-bottom"));

    for (int i = 0; i < ansCnt1.size(); i = i + 4)
    {
        int end = i + 4;
        if (end > ansCnt1.size())
            end = (int)ansCnt1.size();
        if (end + 1 == ansCnt1.size()) {
            end = end + 1;
        }
        sort_contour(ansCnt1, i, end, std::string("left-to-right"));
    }

    for (int i = 0; i < ansCnt2.size(); i = i + 4)
    {
        int end = i + 4;
        if (end > ansCnt2.size())
            end = (int)ansCnt2.size();
        if (end + 1 == ansCnt2.size()) {
            end = end + 1;
        }
        sort_contour(ansCnt2, i, end, std::string("left-to-right"));
    }

    for (int i = 0; i < ansCnt3.size(); i = i + 4)
    {
        int end = i + 4;
        if (end > ansCnt3.size())
            end = (int)ansCnt3.size();
        if (end + 1 == ansCnt3.size()) {
            end = end + 1;
        }
        sort_contour(ansCnt3, i, end, std::string("left-to-right"));
    }

    for (int i = 0; i < ansCnt4.size(); i = i + 4)
    {
        int end = i + 4;
        if (end > ansCnt4.size())
            end = (int)ansCnt4.size();
        if (end + 1 == ansCnt4.size()) {
            end = end + 1;
        }
        sort_contour(ansCnt4, i, end, std::string("left-to-right"));
    }

    for (int i = 0; i < ansCnt5.size(); i = i + 4)
    {
        int end = i + 4;
        if (end > ansCnt5.size())
            end = (int)ansCnt5.size();
        if (end + 1 == ansCnt5.size()) {
            end = end + 1;
        }
        sort_contour(ansCnt5, i, end, std::string("left-to-right"));
    }

    string ans_letters[4] = {"A", "B", "C", "D"};
    string ans1 = "";

    for (int i = 0; i < ansCnt1.size(); i++)
    {
        cv::Rect rectCrop = boundingRect(ansCnt1[i]);
        cv::Mat imageROI(thresh, rectCrop);

        double total = cv::countNonZero(imageROI);
        double pixel = total / rectCrop.area() * 100;

        if (pixel > 65 && pixel < 100)
        {
            rectangle(image, rectCrop.tl(), rectCrop.br(), Scalar(0, 255, 0), 2, 8, 0);
            ans1 = ans1 + "1";
        }
        else {
            rectangle(image, rectCrop.tl(), rectCrop.br(), Scalar(0, 0, 255), 1, 8, 0);
            ans1 = ans1 + "0";
        }
    }

    string ans2 = "";
    for (int i = 0; i < ansCnt2.size(); i++)
    {
        cv::Rect rectCrop = boundingRect(ansCnt2[i]);
        cv::Mat imageROI(thresh, rectCrop);

        double total = cv::countNonZero(imageROI);
        double pixel = total / rectCrop.area() * 100;

        if (pixel > 65 && pixel < 100)
        {
            rectangle(image, rectCrop.tl(), rectCrop.br(), Scalar(0, 255, 0), 2, 8, 0);
            ans2 = ans2 + "1";
        }
        else {
            rectangle(image, rectCrop.tl(), rectCrop.br(), Scalar(0, 0, 255), 1, 8, 0);
            ans2 = ans2 + "0";
        }
    }

    string ans3 = "";
    for (int i = 0; i < ansCnt3.size(); i++)
    {
        cv::Rect rectCrop = boundingRect(ansCnt3[i]);
        cv::Mat imageROI(thresh, rectCrop);

        double total = cv::countNonZero(imageROI);
        double pixel = total / rectCrop.area() * 100;

        if (pixel > 65 && pixel < 100)
        {
            rectangle(image, rectCrop.tl(), rectCrop.br(), Scalar(0, 255, 0), 2, 8, 0);
            ans3 = ans3 + "1";
        }
        else {
            rectangle(image, rectCrop.tl(), rectCrop.br(), Scalar(0, 0, 255), 1, 8, 0);
            ans3 = ans3 + "0";
        }
    }

    string ans4 = "";
    for (int i = 0; i < ansCnt4.size(); i++)
    {
        cv::Rect rectCrop = boundingRect(ansCnt4[i]);
        cv::Mat imageROI(thresh, rectCrop);

        double total = cv::countNonZero(imageROI);
        double pixel = total / rectCrop.area() * 100;

        if (pixel > 65 && pixel < 100)
        {
            rectangle(image, rectCrop.tl(), rectCrop.br(), Scalar(0, 255, 0), 2, 8, 0);
            ans4 = ans4 + "1";
        }
        else {
            rectangle(image, rectCrop.tl(), rectCrop.br(), Scalar(0, 0, 255), 1, 8, 0);
            ans4 = ans4 + "0";
        }
    }

    string ans5 = "";
    for (int i = 0; i < ansCnt5.size(); i++)
    {
        cv::Rect rectCrop = boundingRect(ansCnt5[i]);
        cv::Mat imageROI(thresh, rectCrop);

        double total = cv::countNonZero(imageROI);
        double pixel = total / rectCrop.area() * 100;

        if (pixel > 65 && pixel < 100)
        {
            rectangle(image, rectCrop.tl(), rectCrop.br(), Scalar(0, 255, 0), 2, 8, 0);
            ans5 = ans5 + "1";
        }
        else {
            rectangle(image, rectCrop.tl(), rectCrop.br(), Scalar(0, 0, 255), 1, 8, 0);
            ans5 = ans5 + "0";
        }
    }

    //output
    std::string resultStr = "";
    if(rollCnt.size() == 90 && examCnt.size() == 90 && centerCnt.size() == 50){
        resultStr = rollno + "\n" + examid + "\n" + centerid + "\n" + ans1 + "\n" + ans2 + "\n" + ans3 + "\n" + ans4 + "\n" + ans5;
    }
    else{
        resultStr = "";
    }

    __android_log_print(ANDROID_LOG_VERBOSE, APPNAME, "%s", resultStr.c_str());
    const char *cstr = resultStr.c_str();
    return env->NewStringUTF( cstr);
}

//Binary to Decimal
int binToDec(std::string bin, int first, int length){

    std::string questionNum = bin.substr(first,length);

    int value = 0;
    int indexCounter = 0;
    for(int i=questionNum.length()-1;i>=0;i--){

        if(questionNum[i]=='1'){
            value += pow(2, indexCounter);
        }
        indexCounter++;
    }
    return value;
}

//sort bubble top to bottom
struct sortTopToBottom // 'less' for contours
{
    bool operator ()(const std::vector<cv::Point> &lhs, const std::vector<cv::Point> &rhs)
    {
        cv::Rect rectLhs = boundingRect(cv::Mat(lhs));
        cv::Rect rectRhs = boundingRect(cv::Mat(rhs));
        return rectLhs.y < rectRhs.y;
    }
};

//sort bubble left to right
struct sortLeftToRight // 'less' for contours
{
    bool operator ()(const std::vector<cv::Point> &lhs, const std::vector<cv::Point> &rhs)
    {
        cv::Rect rectLhs = boundingRect(cv::Mat(lhs));
        cv::Rect rectRhs = boundingRect(cv::Mat(rhs));
        return rectLhs.x < rectRhs.x;
    }
};

void sort_contour(std::vector<std::vector<cv::Point> > &contours, int from, int to, std::string sortOrder)
{
    if (!sortOrder.compare("top-to-bottom"))
    {
        sort(contours.begin() + from, contours.begin() + to, sortTopToBottom());
    }
    else if (!sortOrder.compare("left-to-right"))
    {
        sort(contours.begin() + from, contours.begin() + to, sortLeftToRight());
    }
}