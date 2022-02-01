from ubuntu:latest

WORKDIR /home/image_server


ENV DEBIAN_FRONTEND="noninteractive" TZ="America/Los_Angeles"

RUN apt-get update && apt-get -y install  gdb openssh-server rsync zip ffmpeg x264 psmisc sane sane-utils\
 python3-pip cpio  libmysqlclient-dev  git gcc build-essential make libtiff-dev libjpeg-dev\
 php php-mysql libusb-dev cmake libpng-dev zlib1g-dev git libopenjp2-7-dev\
 libtool autoconf automake   xorg-dev  libgl1-mesa-dev  freeglut3-dev tzdata libusb-1.0-0-dev apache2 libxvidcore-dev libapache2-mod-php7.4 fltk1.3-dev



# copy intel ipp here
COPY ./l_ipp_2018.0.128 ./l_ipp_2018.0.128
RUN ./l_ipp_2018.0.128/install.sh -s ./l_ipp_2018.0.128/silent.cfg


RUN python3 -m pip install mysql-connector



COPY ./external_compile_libraries /home/image_server/lifespan/external_compile_libraries


# Build external dependancies.
WORKDIR /home/image_server/lifespan/external_compile_libraries/xvidcore-1.3.4/build/generic
RUN sh ./bootstrap.sh
RUN ./configure
RUN make -j12
RUN make install

WORKDIR /home/image_server/lifespan/external_compile_libraries/fltk
RUN chmod 777 ./configure
RUN  cp misc/config.guess .
RUN cp misc/config.sub .
RUN ./configure --enable-shared
RUN cmake .
RUN make -j12
RUN make install

WORKDIR /home/image_server/lifespan/external_compile_libraries/libdmtx-0.7.4
#RUN ./configure --enable-shared
RUN autoreconf --force --install
RUN ./configure --enable-shared
RUN make . -j12
RUN make install

WORKDIR /home/image_server/lifespan/external_compile_libraries/openjpeg
RUN cmake . -DCmake_BUILD_TYPE=Relase
RUN make -j12
RUN make
RUN make install


WORKDIR /home/image_server/lifespan/external_compile_libraries/sane-backends-1.0.27
RUN ./configure BACKENDS="epson2" RELOADABLE_BACKENDS="epson2"
RUN make
RUN make install



# COPY . /home/image_server/lifespan/
COPY ns_image_server /home/image_server/lifespan/ns_image_server
COPY ns_image_server_utilities /home/image_server/lifespan/ns_image_server_utilities
# COPY external_compile_libraries/* /home/image_server/lifespan/external_compile_libraries/
COPY external_libraries /home/image_server/lifespan/external_libraries
COPY ns_worm_browser /home/image_server/lifespan/ns_worm_browser
COPY binaries /home/image_server/lifespan/binaries
COPY build /home/image_server/lifespan/build
COPY files /home/image_server/lifespan/files

WORKDIR /home/image_server/lifespan/build
RUN cmake . -DBUILD_WORM_BROWSER=1 
RUN make -j12
RUN make install

COPY ./lifespan_startup_scripts/* /home/image_server/
COPY ./linux_lifespan_configs/ns_image_server_website.ini /var/www/html/image_server_web/ns_image_server_website.ini
COPY web_interface /var/www/html/
COPY files/posture_analysis_models /mnt/lifespan_share/long_term_storage/partition_000/posture_analysis_models
COPY files/worm_detection_models /mnt/lifespan_share/long_term_storage/partition_000/worm_detection_models


RUN cp ../files/dll.conf /usr/local/etc/sane.d/dll.d/
RUN cp ../files/epson2.conf /usr/local/etc/sane.d/


RUN chmod 755 /var/www/html/index.php
RUN chmod 755 /var/www/html/image_server_web
RUN chmod 755 /var/www/html/image_server_web/*

RUN a2enmod php7.4


RUN mkdir -p /home/image_server/volatile_storage/lifespan
# GTK needs to write here.
RUN mkdir -p /root/.local/share 


RUN chmod 777 /home/image_server/lifespan_entrypoint.sh


RUN ln -s /mnt/lifespan_share/long_term_storage /var/www/html/
ENV LD_LIBRARY_PATH /opt/intel/compilers_and_libraries_2018.0.128/linux/ipp/lib/intel64_lin/ 

# RUN python3 /home/image_server/entrypoint.py

EXPOSE 80


WORKDIR /home/image_server/lifespan/build/
ENTRYPOINT [ "/home/image_server/lifespan_entrypoint.sh" ]