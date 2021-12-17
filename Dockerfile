FROM ubuntu:latest as base

# Download dependancies and download lifespan repo
WORKDIR /home/image_server


ENV DEBIAN_FRONTEND="noninteractive" TZ="America/Los_Angeles"

RUN apt-get update && apt -y install  gdb openssh-server rsync zip ffmpeg x264 psmisc sane sane-utils\
 python3-pip cpio  libmysqlclient-dev  git gcc build-essential make libtiff-dev libjpeg-dev libfreetype-dev \
 php php-mysql libusb-dev cmake libpng-dev zlib1g-dev git libopenjp2-7-dev\
 libtool autoconf automake   xorg-dev  libgl1-mesa-dev  freeglut3-dev tzdata libusb-1.0-0-dev apache2



# copy intel ipp here
COPY ./l_ipp_2018.0.128 ./l_ipp_2018.0.128
COPY ./lifespan_startup_scripts/* ./
COPY ./linux_lifespan_configs/ns_image_server_website.ini /var/www/html/image_server_web/ns_image_server_website.ini
RUN ./l_ipp_2018.0.128/install.sh -s ./l_ipp_2018.0.128/silent.cfg


RUN python3 -m pip install mysql-connector



COPY ./external_compile_libraries/ /home/image_server/lifespan/external_compile_libraries


# Build external dependancies.
WORKDIR /home/image_server/lifespan/external_compile_libraries/xvidcore-1.3.4/build/generic
RUN sh ./bootstrap.sh
RUN ./configure
RUN make
RUN make install

WORKDIR /home/image_server/lifespan/external_compile_libraries/fltk
RUN chmod 777 ./configure
RUN  cp misc/config.guess .
RUN cp misc/config.sub .
RUN ./configure --enable-shared
RUN cmake .
RUN make install

WORKDIR /home/image_server/lifespan/external_compile_libraries/libdmtx-0.7.4
#RUN ./configure --enable-shared
RUN autoreconf --force --install
RUN ./configure --enable-shared
RUN make .
RUN make install

WORKDIR /home/image_server/lifespan/external_compile_libraries/openjpeg
RUN cmake . -DCmake_BUILD_TYPE=Relase
RUN make
RUN make install


WORKDIR /home/image_server/lifespan/external_compile_libraries/sane-backends-1.0.27
RUN ./configure BACKENDS="epson2" RELOADABLE_BACKENDS="epson2"
RUN make
RUN make install



COPY . /home/image_server/lifespan

WORKDIR /home/image_server/lifespan/build
RUN cmake . 
RUN make -j12
RUN make install

RUN cp ../files/dll.conf /usr/local/etc/sane.d/dll.d/
RUN cp ../files/epson2.conf /usr/local/etc/sane.d/

RUN cp -r /home/image_server/lifespan/web_interface/* /var/www/html/

RUN chmod 755 /var/www/html/index.php
RUN chmod 755 /var/www/html/image_server_web
RUN chmod 755 /var/www/html/image_server_web/*



RUN mkdir /mnt/lifespan_share
RUN mkdir /mnt/lifespan_share/long_term_storage
RUN mkdir /home/image_server/volatile_storage


RUN chmod 777 /home/image_server/lifespan_entrypoint.sh




RUN ln -s /mnt/lifespan_share/long_term_storage /var/www/html/



RUN python3 /home/image_server/entrypoint.py


# EXPOSE 80
# CMD [ "/home/image_server/lifespan_entrypoint.sh" ]

WORKDIR /home/image_server/lifespan/build/
ENTRYPOINT [ "/home/image_server/lifespan_entrypoint.sh" ]

# ENTRYPOINT [ "tail", "-f","/dev/null" ]

