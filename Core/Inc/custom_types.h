#ifndef INC_CUSTOM_TYPES_H_
#define INC_CUSTOM_TYPES_H_

typedef struct {
    int16_t ax;
    int16_t ay;
    int16_t az;
    int16_t gx;
    int16_t gy;
    int16_t gz;
    uint32_t timestamp;
} BMI2_Data;

typedef struct {
    float latitude;
    float longitude;
    float altitude;
} GPS_Fix_t;

#endif // CUSTOM_TYPES_H_
